#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <zlib.h>

#define BOOT_MAGIC        "ANDROID!"
#define BOOT_MAGIC_SIZE   8
#define BOOT_NAME_SIZE    16
#define BOOT_ARGS_SIZE    512
#define BOOT_EXTRA_ARGS_SIZE 1024
#define PAGE_SIZE_DEFAULT 2048

#define COMP_NONE  0
#define COMP_GZIP  1
#define COMP_LZ4F  2
#define COMP_LZ4L  3
#define COMP_XZ    4
#define COMP_BZIP2 5

typedef struct {
    uint8_t  magic[BOOT_MAGIC_SIZE];
    uint32_t kernel_size;
    uint32_t kernel_addr;
    uint32_t ramdisk_size;
    uint32_t ramdisk_addr;
    uint32_t second_size;
    uint32_t second_addr;
    uint32_t tags_addr;
    uint32_t page_size;
    uint32_t header_version;
    uint32_t os_version;
    uint8_t  name[BOOT_NAME_SIZE];
    uint8_t  cmdline[BOOT_ARGS_SIZE];
    uint32_t id[8];
    uint8_t  extra_cmdline[BOOT_EXTRA_ARGS_SIZE];
    uint32_t recovery_dtbo_size;
    uint64_t recovery_dtbo_offset;
    uint32_t header_size;
} boot_img_hdr;

static uint8_t *read_file(const char *path, size_t *sz) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    *sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *buf = (uint8_t *)malloc(*sz);
    if (!buf) { fclose(f); return NULL; }
    fread(buf, 1, *sz, f);
    fclose(f);
    return buf;
}

static int write_file(const char *path, const uint8_t *data, size_t size) {
    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    fwrite(data, 1, size, f);
    fclose(f);
    return 0;
}

static int detect_comp(const uint8_t *d, size_t sz) {
    if (sz < 4) return COMP_NONE;
    if (d[0]==0x1f && d[1]==0x8b) return COMP_GZIP;
    if (d[0]==0x02 && d[1]==0x21 && d[2]==0x4c && d[3]==0x18) return COMP_LZ4F;
    if (d[0]==0x04 && d[1]==0x22 && d[2]==0x4d && d[3]==0x18) return COMP_LZ4F;
    if (d[0]==0x02 && d[1]==0x21) return COMP_LZ4L;
    if (sz>=8 && d[0]==0x02 && d[1]==0x21) return COMP_LZ4L;
    if (d[0]==0xfd && d[1]==0x37 && d[2]==0x7a) return COMP_XZ;
    if (d[0]==0x42 && d[1]==0x5a && d[2]==0x68) return COMP_BZIP2;
    return COMP_NONE;
}

static int gz_decompress(const uint8_t *in, size_t in_sz, uint8_t **out, size_t *out_sz) {
    size_t buf_sz = in_sz * 8 + 4096;
    uint8_t *buf = (uint8_t *)malloc(buf_sz);
    if (!buf) return -1;
    z_stream s;
    memset(&s, 0, sizeof(s));
    s.next_in = (Bytef *)in;
    s.avail_in = in_sz;
    if (inflateInit2(&s, 15+16) != Z_OK) { free(buf); return -1; }
    s.next_out = buf;
    s.avail_out = buf_sz;
    int r;
    while ((r = inflate(&s, Z_NO_FLUSH)) == Z_OK) {
        if (s.avail_out == 0) {
            buf_sz *= 2;
            uint8_t *t = (uint8_t *)realloc(buf, buf_sz);
            if (!t) { inflateEnd(&s); free(buf); return -1; }
            buf = t;
            s.next_out = buf + s.total_out;
            s.avail_out = buf_sz - s.total_out;
        }
    }
    *out_sz = s.total_out;
    inflateEnd(&s);
    if (r != Z_STREAM_END) { free(buf); return -1; }
    *out = buf;
    return 0;
}

static int gz_compress(const uint8_t *in, size_t in_sz, uint8_t **out, size_t *out_sz) {
    size_t buf_sz = in_sz + (in_sz/10) + 128;
    uint8_t *buf = (uint8_t *)malloc(buf_sz);
    if (!buf) return -1;
    z_stream s;
    memset(&s, 0, sizeof(s));
    if (deflateInit2(&s, Z_BEST_COMPRESSION, Z_DEFLATED, 15+16, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        free(buf); return -1;
    }
    s.next_in = (Bytef *)in;
    s.avail_in = in_sz;
    s.next_out = buf;
    s.avail_out = buf_sz;
    deflate(&s, Z_FINISH);
    *out_sz = s.total_out;
    deflateEnd(&s);
    *out = buf;
    return 0;
}

static int lz4_block_decompress(const uint8_t *src, size_t src_sz, uint8_t **dst, size_t *dst_sz) {
    size_t buf_sz = src_sz * 8 + 4096;
    uint8_t *buf = (uint8_t *)malloc(buf_sz);
    if (!buf) return -1;
    const uint8_t *p = src, *end = src + src_sz;
    size_t out = 0;
    while (p < end) {
        if (p + 4 > end) break;
        uint32_t bsz = p[0]|(p[1]<<8)|(p[2]<<16)|((uint32_t)p[3]<<24);
        p += 4;
        if (bsz == 0) break;
        int uncompressed = (bsz & 0x80000000) != 0;
        bsz &= 0x7fffffff;
        if (p + bsz > end) break;
        if (uncompressed) {
            while (out + bsz > buf_sz) { buf_sz *= 2; uint8_t *t = (uint8_t*)realloc(buf,buf_sz); if(!t){free(buf);return -1;} buf=t; }
            memcpy(buf+out, p, bsz); out += bsz;
        } else {
            const uint8_t *ip = p, *ie = p + bsz;
            while (ip < ie) {
                uint8_t tok = *ip++;
                size_t ll = (tok >> 4) & 0xf;
                if (ll == 15) { uint8_t x; do { x = *ip++; ll += x; } while (x==255 && ip<ie); }
                while (out+ll > buf_sz) { buf_sz*=2; uint8_t *t=(uint8_t*)realloc(buf,buf_sz); if(!t){free(buf);return -1;} buf=t; }
                memcpy(buf+out, ip, ll); out += ll; ip += ll;
                if (ip >= ie) break;
                uint16_t off = ip[0]|(ip[1]<<8); ip += 2;
                if (off == 0) break;
                size_t ml = (tok & 0xf) + 4;
                if ((ml-4) == 15) { uint8_t x; do { x = *ip++; ml += x; } while (x==255 && ip<ie); }
                while (out+ml > buf_sz) { buf_sz*=2; uint8_t *t=(uint8_t*)realloc(buf,buf_sz); if(!t){free(buf);return -1;} buf=t; }
                size_t cs = out - off;
                for (size_t i = 0; i < ml; i++) buf[out++] = buf[cs+i];
            }
        }
        p += bsz;
    }
    *dst = buf; *dst_sz = out;
    return 0;
}

static int lz4_decompress(const uint8_t *src, size_t src_sz, uint8_t **dst, size_t *dst_sz) {
    if (src_sz < 4) return -1;
    if (src[0]==0x02 && src[1]==0x21 && src[2]==0x4c && src[3]==0x18) {
        return lz4_block_decompress(src + 7, src_sz - 7, dst, dst_sz);
    }
    if (src[0]==0x04 && src[1]==0x22 && src[2]==0x4d && src[3]==0x18) {
        return lz4_block_decompress(src + 7, src_sz - 7, dst, dst_sz);
    }
    size_t buf_sz = src_sz * 8 + 4096;
    uint8_t *buf = (uint8_t *)malloc(buf_sz);
    if (!buf) return -1;
    const uint8_t *p = src, *end = src + src_sz;
    size_t out = 0;
    while (p + 4 <= end) {
        uint32_t chunk = p[0]|(p[1]<<8)|(p[2]<<16)|((uint32_t)p[3]<<24);
        p += 4;
        if (chunk == 0x184c2102) continue;
        if (p + chunk > end) break;
        const uint8_t *ip = p, *ie = p + chunk;
        while (ip < ie) {
            uint8_t tok = *ip++;
            size_t ll = (tok >> 4) & 0xf;
            if (ll == 15) { uint8_t x; do { x = *ip++; ll += x; } while (x==255 && ip<ie); }
            while (out+ll > buf_sz) { buf_sz*=2; uint8_t *t=(uint8_t*)realloc(buf,buf_sz); if(!t){free(buf);return -1;} buf=t; }
            memcpy(buf+out, ip, ll); out += ll; ip += ll;
            if (ip >= ie) break;
            uint16_t off = ip[0]|(ip[1]<<8); ip += 2;
            if (off == 0) break;
            size_t ml = (tok & 0xf) + 4;
            if ((ml-4) == 15) { uint8_t x; do { x = *ip++; ml += x; } while (x==255 && ip<ie); }
            while (out+ml > buf_sz) { buf_sz*=2; uint8_t *t=(uint8_t*)realloc(buf,buf_sz); if(!t){free(buf);return -1;} buf=t; }
            size_t cs = out - off;
            for (size_t i = 0; i < ml; i++) buf[out++] = buf[cs+i];
        }
        p += chunk;
    }
    *dst = buf; *dst_sz = out;
    return 0;
}

static size_t pg_align(size_t sz, uint32_t pg) { return ((sz + pg - 1) / pg) * pg; }

static void inject_init_to_cpio(uint8_t **cpio, size_t *cpio_sz, const char *init_path) {
    size_t isz = 0;
    uint8_t *idata = read_file(init_path, &isz);
    if (!idata || isz == 0) {
        fprintf(stderr, "Warning: %s not found, skipping injection\n", init_path);
        return;
    }
    const char *fname = "init_real";
    size_t nlen = strlen(fname);
    char hdr[110];
    snprintf(hdr, sizeof(hdr),
        "070701%08X%08X%08X%08X%08X%08X%08X%08X%08X%08X%08X%08X%08X",
        0x1, 0x81ED, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0,
        (uint32_t)isz, 0x0, 0x0, (uint32_t)(nlen+1), 0x0);
    size_t pos = 0;
    size_t hpad = ((110 + nlen + 1 + 3) / 4) * 4;
    size_t dpad = ((isz + 3) / 4) * 4;
    size_t entry_sz = hpad + dpad;
    uint8_t *new_cpio = (uint8_t *)calloc(1, *cpio_sz + entry_sz);
    if (!new_cpio) { free(idata); return; }
    memcpy(new_cpio + pos, hdr, 110); pos += 110;
    memcpy(new_cpio + pos, fname, nlen + 1); pos += nlen + 1;
    while (pos % 4) pos++;
    memcpy(new_cpio + pos, idata, isz); pos += isz;
    while (pos % 4) pos++;
    memcpy(new_cpio + pos, *cpio, *cpio_sz);
    free(*cpio); free(idata);
    *cpio = new_cpio;
    *cpio_sz = pos + *cpio_sz;
}

int cmd_patch(const char *boot_path, const char *init_path, const char *out_path) {
    size_t img_sz = 0;
    uint8_t *img = read_file(boot_path, &img_sz);
    if (!img) { fprintf(stderr, "Cannot read boot image: %s\n", boot_path); return 1; }
    if (memcmp(img, BOOT_MAGIC, BOOT_MAGIC_SIZE) != 0) {
        fprintf(stderr, "Invalid boot image magic\n"); free(img); return 1;
    }
    boot_img_hdr *hdr = (boot_img_hdr *)img;
    uint32_t pg = hdr->page_size > 0 ? hdr->page_size : PAGE_SIZE_DEFAULT;
    printf("page_size=%u kernel_size=%u ramdisk_size=%u hdr_ver=%u\n",
        pg, hdr->kernel_size, hdr->ramdisk_size, hdr->header_version);
    size_t koff = pg;
    size_t rdoff = koff + pg_align(hdr->kernel_size, pg);
    size_t s2off = rdoff + pg_align(hdr->ramdisk_size, pg);
    uint8_t *rd_raw = img + rdoff;
    size_t   rd_raw_sz = hdr->ramdisk_size;
    int comp = detect_comp(rd_raw, rd_raw_sz);
    const char *comp_name[] = { "none/raw", "gzip", "lz4-framed", "lz4-legacy", "xz", "bzip2" };
    printf("ramdisk_compression=%s\n", comp_name[comp]);
    uint8_t *cpio = NULL;
    size_t   cpio_sz = 0;
    switch (comp) {
        case COMP_GZIP:
            if (gz_decompress(rd_raw, rd_raw_sz, &cpio, &cpio_sz) != 0) {
                fprintf(stderr, "gzip decompress failed\n"); free(img); return 1;
            }
            break;
        case COMP_LZ4F:
        case COMP_LZ4L:
            if (lz4_decompress(rd_raw, rd_raw_sz, &cpio, &cpio_sz) != 0) {
                fprintf(stderr, "lz4 decompress failed\n"); free(img); return 1;
            }
            break;
        default:
            cpio = (uint8_t *)malloc(rd_raw_sz);
            memcpy(cpio, rd_raw, rd_raw_sz);
            cpio_sz = rd_raw_sz;
            break;
    }
    printf("cpio_size=%zu\n", cpio_sz);
    inject_init_to_cpio(&cpio, &cpio_sz, init_path);
    uint8_t *new_rd = NULL;
    size_t   new_rd_sz = 0;
    if (comp == COMP_GZIP) {
        if (gz_compress(cpio, cpio_sz, &new_rd, &new_rd_sz) != 0) {
            fprintf(stderr, "gzip compress failed\n"); free(cpio); free(img); return 1;
        }
    } else {
        new_rd = (uint8_t *)malloc(cpio_sz);
        memcpy(new_rd, cpio, cpio_sz);
        new_rd_sz = cpio_sz;
    }
    free(cpio);
    size_t new_sz = pg
        + pg_align(hdr->kernel_size, pg)
        + pg_align(new_rd_sz, pg)
        + pg_align(hdr->second_size, pg);
    uint8_t *out = (uint8_t *)calloc(1, new_sz);
    if (!out) { free(img); free(new_rd); return 1; }
    memcpy(out, img, pg);
    ((boot_img_hdr *)out)->ramdisk_size = (uint32_t)new_rd_sz;
    memcpy(out + koff, img + koff, hdr->kernel_size);
    memcpy(out + rdoff, new_rd, new_rd_sz);
    if (hdr->second_size > 0)
        memcpy(out + s2off, img + s2off, hdr->second_size);
    write_file(out_path, out, new_sz);
    printf("patched_output=%s size=%zu\n", out_path, new_sz);
    free(img); free(new_rd); free(out);
    return 0;
}

int cmd_info(const char *boot_path) {
    size_t sz = 0;
    uint8_t *img = read_file(boot_path, &sz);
    if (!img) { fprintf(stderr, "Cannot read: %s\n", boot_path); return 1; }
    if (memcmp(img, BOOT_MAGIC, BOOT_MAGIC_SIZE) != 0) {
        fprintf(stderr, "Not a boot image\n"); free(img); return 1;
    }
    boot_img_hdr *h = (boot_img_hdr *)img;
    uint32_t pg = h->page_size > 0 ? h->page_size : PAGE_SIZE_DEFAULT;
    size_t rdoff = pg + pg_align(h->kernel_size, pg);
    int comp = detect_comp(img + rdoff, h->ramdisk_size);
    const char *cn[] = { "none", "gzip", "lz4_framed", "lz4_legacy", "xz", "bzip2" };
    printf("{\n");
    printf("  \"kernel_size\": %u,\n", h->kernel_size);
    printf("  \"ramdisk_size\": %u,\n", h->ramdisk_size);
    printf("  \"page_size\": %u,\n", pg);
    printf("  \"header_version\": %u,\n", h->header_version);
    printf("  \"compression\": \"%s\",\n", cn[comp]);
    printf("  \"name\": \"%s\",\n", h->name);
    printf("  \"cmdline\": \"%s\"\n", h->cmdline);
    printf("}\n");
    free(img);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "bootpatch patch <boot.img> <init> <out.img>\n");
        fprintf(stderr, "bootpatch info  <boot.img>\n");
        return 1;
    }
    if (strcmp(argv[1], "patch") == 0 && argc >= 5) return cmd_patch(argv[2], argv[3], argv[4]);
    if (strcmp(argv[1], "info")  == 0)               return cmd_info(argv[2]);
    fprintf(stderr, "Unknown command\n");
    return 1;
}
