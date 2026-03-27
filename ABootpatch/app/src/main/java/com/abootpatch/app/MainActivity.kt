package com.abootpatch.app

import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import androidx.navigation.findNavController
import androidx.navigation.ui.setupWithNavController
import com.abootpatch.app.core.ABootService
import com.abootpatch.app.databinding.ActivityMainBinding
import com.google.android.material.bottomnavigation.BottomNavigationView

class MainActivity : AppCompatActivity() {
    private lateinit var binding: ActivityMainBinding
    override fun onCreate(s: Bundle?) {
        super.onCreate(s)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)
        val nav: BottomNavigationView = binding.navView
        nav.setupWithNavController(findNavController(R.id.nav_host_fragment_activity_main))
        ABootService.init(this)
    }
}
