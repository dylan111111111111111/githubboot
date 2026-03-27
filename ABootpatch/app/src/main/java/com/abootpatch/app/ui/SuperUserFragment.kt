package com.abootpatch.app.ui

import android.os.Bundle
import android.view.*
import androidx.fragment.app.Fragment
import androidx.lifecycle.ViewModelProvider
import androidx.recyclerview.widget.LinearLayoutManager
import com.abootpatch.app.databinding.FragmentSuperuserBinding
import com.abootpatch.app.modules.SuperUserAdapter

class SuperUserFragment : Fragment() {
    private var _b: FragmentSuperuserBinding? = null
    private val b get() = _b!!
    private lateinit var vm: SuperUserViewModel

    override fun onCreateView(i: LayoutInflater, c: ViewGroup?, s: Bundle?): View {
        vm = ViewModelProvider(this)[SuperUserViewModel::class.java]
        _b = FragmentSuperuserBinding.inflate(i, c, false)
        return b.root
    }

    override fun onViewCreated(v: View, s: Bundle?) {
        super.onViewCreated(v, s)
        val adapter = SuperUserAdapter { app, on -> vm.setPermission(app.packageName, on) }
        b.recyclerSuperuser.layoutManager = LinearLayoutManager(requireContext())
        b.recyclerSuperuser.adapter = adapter
        vm.apps.observe(viewLifecycleOwner) { adapter.submitList(it) }
        vm.load()
    }

    override fun onDestroyView() { super.onDestroyView(); _b = null }
}
