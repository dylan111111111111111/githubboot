package com.abootpatch.app.ui

import android.os.Bundle
import android.view.*
import androidx.fragment.app.Fragment
import androidx.lifecycle.ViewModelProvider
import androidx.recyclerview.widget.LinearLayoutManager
import com.abootpatch.app.databinding.FragmentModulesBinding
import com.abootpatch.app.modules.ModuleAdapter

class ModulesFragment : Fragment() {
    private var _b: FragmentModulesBinding? = null
    private val b get() = _b!!
    private lateinit var vm: ModulesViewModel

    override fun onCreateView(i: LayoutInflater, c: ViewGroup?, s: Bundle?): View {
        vm = ViewModelProvider(this)[ModulesViewModel::class.java]
        _b = FragmentModulesBinding.inflate(i, c, false)
        return b.root
    }

    override fun onViewCreated(v: View, s: Bundle?) {
        super.onViewCreated(v, s)
        val adapter = ModuleAdapter(onToggle={m,on->vm.toggle(m.id,on)}, onDelete={m->vm.delete(m.id)})
        b.recyclerModules.layoutManager = LinearLayoutManager(requireContext())
        b.recyclerModules.adapter = adapter
        vm.modules.observe(viewLifecycleOwner) { adapter.submitList(it) }
        b.fabInstall.setOnClickListener { vm.pickZip(requireActivity()) }
        vm.load()
    }

    override fun onDestroyView() { super.onDestroyView(); _b = null }
}
