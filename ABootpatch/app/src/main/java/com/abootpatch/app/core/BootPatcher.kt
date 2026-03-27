package com.abootpatch.app.core

import com.topjohnwu.superuser.Shell

data class PatchResult(val success: Boolean, val message: String)

object BootPatcher {
    private const val TOOL = "/data/adb/abootpatch/bootpatch"
    private const val INIT = "/data/adb/abootpatch/abootpatchinit"
    private const val DUMP = "/data/adb/abootpatch/boot_dump.img"
    private const val PATCHED = "/data/adb/abootpatch/boot_patched.img"

    fun getInfo(path: String): String {
        val r = Shell.cmd("$TOOL info $path").exec()
        return if (r.isSuccess) r.out.joinToString("\n") else r.err.joinToString("\n")
    }

    fun detectSlot(): String = Shell.cmd("getprop ro.boot.slot_suffix").exec().out.firstOrNull()?.trim() ?: ""

    fun findBootPartition(): String? {
        val slot = detectSlot()
        for (p in listOf(
            "/dev/block/by-name/boot$slot",
            "/dev/block/bootdevice/by-name/boot$slot",
            "/dev/block/platform/*/by-name/boot$slot"
        )) {
            val r = Shell.cmd("readlink -f $p 2>/dev/null").exec()
            if (r.isSuccess && r.out.isNotEmpty() && r.out.first().isNotBlank()) return r.out.first().trim()
        }
        val scan = Shell.cmd("find /dev/block -name 'boot*' 2>/dev/null | head -1").exec()
        return if (scan.isSuccess && scan.out.isNotEmpty()) scan.out.first().trim() else null
    }

    fun patchAndFlash(): PatchResult {
        val part = findBootPartition() ?: return PatchResult(false, "Boot partition not found")
        val dump = Shell.cmd("dd if=$part of=$DUMP bs=4096 2>&1").exec()
        if (!dump.isSuccess) return PatchResult(false, "Failed to dump boot: ${dump.err.joinToString()}")
        val info = Shell.cmd("$TOOL info $DUMP").exec()
        val patch = Shell.cmd("$TOOL patch $DUMP $INIT $PATCHED 2>&1").exec()
        if (!patch.isSuccess) return PatchResult(false, "Patch failed: ${patch.err.joinToString("\n")}")
        val flash = Shell.cmd("dd if=$PATCHED of=$part bs=4096 2>&1").exec()
        Shell.cmd("rm -f $DUMP $PATCHED").exec()
        return if (flash.isSuccess) PatchResult(true, "Patched and flashed successfully!\n${info.out.joinToString("\n")}")
               else PatchResult(false, "Failed to flash: ${flash.err.joinToString()}")
    }

    fun patchFile(inputPath: String, outputPath: String): PatchResult {
        val r = Shell.cmd("$TOOL patch $inputPath $INIT $outputPath 2>&1").exec()
        return if (r.isSuccess) PatchResult(true, r.out.joinToString("\n"))
               else PatchResult(false, r.err.joinToString("\n").ifEmpty { r.out.joinToString("\n") })
    }
}
