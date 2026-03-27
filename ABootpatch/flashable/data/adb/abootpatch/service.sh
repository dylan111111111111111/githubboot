#!/system/bin/sh
MODDIR=/data/adb/modules
until [ -d /sdcard ]; do sleep 1; done
for d in "$MODDIR"/*/; do
    [ -f "$d/disable" ] && continue
    [ -f "$d/service.sh" ] && sh "$d/service.sh" &
done
