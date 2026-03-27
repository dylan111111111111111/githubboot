package com.abootpatch.app.ui

import android.app.AlertDialog
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.fragment.app.Fragment
import androidx.lifecycle.ViewModelProvider
import com.abootpatch.app.R
import com.abootpatch.app.databinding.FragmentHomeBinding

class HomeFragment : Fragment() {
    private var _b: FragmentHomeBinding? = null
    private val b get() = _b!!
    private lateinit var vm: HomeViewModel

    override fun onCreateView(i: LayoutInflater, c: ViewGroup?, s: Bundle?): View {
        vm = ViewModelProvider(this)[HomeViewModel::class.java]
        _b = FragmentHomeBinding.inflate(i, c, false)
        return b.root
    }

    override fun onViewCreated(v: View, s: Bundle?) {
        super.onViewCreated(v, s)
        vm.statusText.observe(viewLifecycleOwner) { b.textStatus.text = it }
        vm.rooted.observe(viewLifecycleOwner) { ok ->
            b.cardStatus.setCardBackgroundColor(requireContext().getColor(if(ok) R.color.green_500 else R.color.red_500))
        }
        vm.patchMsg.observe(viewLifecycleOwner) { msg ->
            if (msg.isNotEmpty()) {
                AlertDialog.Builder(requireContext()).setTitle("Result").setMessage(msg).setPositiveButton("OK",null).show()
                vm.clearMsg()
            }
        }
        b.btnPatchBoot.setOnClickListener {
            AlertDialog.Builder(requireContext())
                .setTitle("Patch Boot")
                .setMessage("Patch and flash boot partition?")
                .setPositiveButton("Patch") { _,_ -> vm.patchAndFlash() }
                .setNegativeButton("Cancel",null).show()
        }
        b.btnReboot.setOnClickListener {
            AlertDialog.Builder(requireContext())
                .setTitle("Reboot")
                .setItems(arrayOf("Reboot","Recovery","Bootloader")) { _,w ->
                    vm.reboot(when(w){1->"recovery";2->"bootloader";else->""})
                }.show()
        }
        vm.refresh()
    }

    override fun onDestroyView() { super.onDestroyView(); _b = null }
}
