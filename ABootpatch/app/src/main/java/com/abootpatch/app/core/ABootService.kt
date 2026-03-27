package com.abootpatch.app.core

import android.content.Context
import android.net.LocalSocket
import android.net.LocalSocketAddress
import java.io.DataInputStream
import java.io.DataOutputStream

object ABootService {
    private const val SOCKET = "/dev/socket/abootpatch_daemon"
    private const val REQ_STATUS = 1
    private const val REQ_ROOT   = 2
    private const val REQ_REBOOT = 4

    fun init(ctx: Context) { DaemonManager.startIfNeeded(ctx) }

    private fun sock(): LocalSocket {
        val s = LocalSocket()
        s.connect(LocalSocketAddress(SOCKET, LocalSocketAddress.Namespace.FILESYSTEM))
        return s
    }

    fun isRooted(): Boolean = try {
        val s = sock()
        val o = DataOutputStream(s.outputStream); val i = DataInputStream(s.inputStream)
        o.writeInt(REQ_STATUS); o.flush(); val r = i.readInt(); s.close(); r == 0
    } catch (e: Exception) { false }

    fun requestRoot(pkg: String): Boolean = try {
        val s = sock()
        val o = DataOutputStream(s.outputStream); val i = DataInputStream(s.inputStream)
        val b = pkg.toByteArray()
        o.writeInt(REQ_ROOT); o.writeInt(b.size); o.write(b); o.flush()
        val r = i.readInt(); s.close(); r == 0
    } catch (e: Exception) { false }

    fun reboot(mode: String = "") = try {
        val s = sock(); val o = DataOutputStream(s.outputStream)
        val b = mode.toByteArray()
        o.writeInt(REQ_REBOOT); o.writeInt(b.size); o.write(b); o.flush(); s.close()
    } catch (e: Exception) {}
}
