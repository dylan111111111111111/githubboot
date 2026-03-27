package com.abootpatch.app.ui

import androidx.lifecycle.*
import com.abootpatch.app.core.ABootService
import com.abootpatch.app.core.BootPatcher
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

class HomeViewModel : ViewModel() {
    private val _status = MutableLiveData<String>()
    val statusText: LiveData<String> = _status
    private val _rooted = MutableLiveData<Boolean>()
    val rooted: LiveData<Boolean> = _rooted
    private val _msg = MutableLiveData<String>()
    val patchMsg: LiveData<String> = _msg

    fun clearMsg() { _msg.value = "" }

    fun refresh() = viewModelScope.launch(Dispatchers.IO) {
        val ok = ABootService.isRooted()
        val slot = BootPatcher.detectSlot()
        val part = BootPatcher.findBootPartition()
        withContext(Dispatchers.Main) {
            _rooted.value = ok
            _status.value = if (ok) "Active | Slot: ${slot.ifEmpty{"Single"}} | $part"
                            else "Not installed — tap Patch Boot"
        }
    }

    fun patchAndFlash() = viewModelScope.launch(Dispatchers.IO) {
        withContext(Dispatchers.Main) { _status.value = "Patching..." }
        val r = BootPatcher.patchAndFlash()
        withContext(Dispatchers.Main) { _msg.value = r.message; refresh() }
    }

    fun reboot(mode: String) = viewModelScope.launch(Dispatchers.IO) { ABootService.reboot(mode) }
}
