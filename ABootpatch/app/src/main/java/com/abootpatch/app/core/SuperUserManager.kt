package com.abootpatch.app.core

import com.topjohnwu.superuser.Shell

data class RootApp(val packageName: String, val uid: Int, val allowed: Boolean)

object SuperUserManager {
    private const val DIR = "/data/adb/abootpatch/policy"

    fun getGrantedApps(): List<RootApp> {
        Shell.cmd("mkdir -p $DIR").exec()
        val r = Shell.cmd("ls $DIR 2>/dev/null").exec()
        if (!r.isSuccess) return emptyList()
        return r.out.filter { it.isNotBlank() }.mapNotNull { pkg ->
            runCatching {
                val c = Shell.cmd("cat $DIR/$pkg").exec()
                var uid = 0; var allowed = false
                c.out.forEach { line ->
                    when { line.startsWith("uid=") -> uid = line.substringAfter("=").toIntOrNull()?:0
                           line.startsWith("policy=") -> allowed = line.substringAfter("=")=="allow" }
                }
                RootApp(pkg, uid, allowed)
            }.getOrNull()
        }
    }

    fun setPermission(pkg: String, allow: Boolean) {
        Shell.cmd("mkdir -p $DIR").exec()
        val uid = Shell.cmd("dumpsys package $pkg 2>/dev/null | grep userId= | head -1 | sed 's/.*userId=//' | tr -d ' '")
            .exec().out.firstOrNull()?.trim() ?: "0"
        Shell.cmd("printf 'package=%s\\nuid=%s\\npolicy=%s\\n' '$pkg' '$uid' '${if(allow)"allow"else"deny"}' > $DIR/$pkg").exec()
    }
}
