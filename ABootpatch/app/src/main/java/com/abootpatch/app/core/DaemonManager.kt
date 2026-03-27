package com.abootpatch.app.core

import android.content.Context
import com.topjohnwu.superuser.Shell
import java.io.File

object DaemonManager {
    private const val DIR = "/data/adb/abootpatch"

    fun startIfNeeded(ctx: Context) {
        Shell.getShell { extractAll(ctx); if (!running()) launch() }
    }

    private fun running() = Shell.cmd("pgrep -f abootpatchd").exec().let { it.isSuccess && it.out.isNotEmpty() }

    private fun extractAll(ctx: Context) {
        File(DIR).mkdirs()
        listOf("abootpatchd","bootpatch","abootpatchinit","su").forEach { name ->
            runCatching {
                val target = File(DIR, name)
                ctx.assets.open(name).use { src -> target.outputStream().use { src.copyTo(it) } }
                Shell.cmd("chmod 755 ${target.absolutePath}").exec()
            }
        }
    }

    private fun launch() { Shell.cmd("$DIR/abootpatchd &").exec() }
}
