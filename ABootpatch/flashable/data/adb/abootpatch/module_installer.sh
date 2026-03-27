#!/system/bin/sh
ZIPFILE="$1"
TMPDIR=/data/adb/abootpatch/tmp_mod
MODDIR=/data/adb/modules
[ -z "$ZIPFILE" ] || [ ! -f "$ZIPFILE" ] && exit 1
rm -rf "$TMPDIR"; mkdir -p "$TMPDIR"
unzip -o "$ZIPFILE" -d "$TMPDIR" || exit 1
ID=$(grep "^id=" "$TMPDIR/module.prop" 2>/dev/null | cut -d= -f2-)
[ -z "$ID" ] && exit 1
mkdir -p "$MODDIR/$ID"
cp -rf "$TMPDIR/." "$MODDIR/$ID/"
chmod -R 755 "$MODDIR/$ID"
[ -f "$MODDIR/$ID/post-fs-data.sh" ] && sh "$MODDIR/$ID/post-fs-data.sh"
rm -rf "$TMPDIR"
exit 0
