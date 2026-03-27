package com.abootpatch.app.ui

import android.app.Activity
import androidx.lifecycle.*
import com.abootpatch.app.core.ABootModule
import com.abootpatch.app.core.ModuleManager
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

class ModulesViewModel : ViewModel() {
    private val _m = MutableLiveData<List<ABootModule>>()
    val modules: LiveData<List<ABootModule>> = _m

    fun load() = viewModelScope.launch(Dispatchers.IO) {
        val list = ModuleManager.loadModules()
        withContext(Dispatchers.Main) { _m.value = list }
    }
    fun toggle(id: String, on: Boolean) = viewModelScope.launch(Dispatchers.IO) { ModuleManager.toggleModule(id,on); load() }
    fun delete(id: String) = viewModelScope.launch(Dispatchers.IO) { ModuleManager.removeModule(id); load() }
    fun pickZip(activity: Activity) {}
}
