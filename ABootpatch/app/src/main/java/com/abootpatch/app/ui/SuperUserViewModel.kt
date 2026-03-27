package com.abootpatch.app.ui

import androidx.lifecycle.*
import com.abootpatch.app.core.RootApp
import com.abootpatch.app.core.SuperUserManager
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

class SuperUserViewModel : ViewModel() {
    private val _a = MutableLiveData<List<RootApp>>()
    val apps: LiveData<List<RootApp>> = _a
    fun load() = viewModelScope.launch(Dispatchers.IO) { withContext(Dispatchers.Main) { _a.value = SuperUserManager.getGrantedApps() } }
    fun setPermission(pkg: String, allow: Boolean) = viewModelScope.launch(Dispatchers.IO) { SuperUserManager.setPermission(pkg,allow); load() }
}
