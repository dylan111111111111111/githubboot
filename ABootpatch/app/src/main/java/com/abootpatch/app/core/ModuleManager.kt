package com.abootpatch.app.core

import com.topjohnwu.superuser.Shell
import java.util.Properties

data class ABootModule(val id:String, val name:String, val version:String, val author:String, val description:String, val enabled:Boolean)

object ModuleManager {
    private const val DIR = "/data/adb/modules"

    fun loadModules(): List<ABootModule> {
        val r = Shell.cmd("ls $DIR 2>/dev/null").exec()
        if (!r.isSuccess) return emptyList()
        return r.out.filter { it.isNotBlank() }.mapNotNull { id -> readProps("$DIR/$id", id) }
    }

    private fun readProps(path: String, id: String): ABootModule? = runCatching {
        val r = Shell.cmd("cat $path/module.prop 2>/dev/null").exec()
        val p = Properties()
        r.out.forEach { line -> val i = line.indexOf('='); if(i>=0) p[line.substring(0,i).trim()]=line.substring(i+1).trim() }
        val dis = Shell.cmd("[ -f $path/disable ] && echo 1 || echo 0").exec().out.firstOrNull() == "1"
        ABootModule(p.getProperty("id",id), p.getProperty("name",id), p.getProperty("version",""), p.getProperty("author",""), p.getProperty("description",""), !dis)
    }.getOrNull()

    fun toggleModule(id: String, on: Boolean) {
        val f = "$DIR/$id/disable"
        if (on) Shell.cmd("rm -f $f").exec() else Shell.cmd("touch $f").exec()
    }

    fun installModule(zip: String): Boolean = Shell.cmd("/data/adb/abootpatch/module_installer.sh $zip").exec().isSuccess
    fun removeModule(id: String): Boolean = Shell.cmd("rm -rf $DIR/$id").exec().isSuccess
}
