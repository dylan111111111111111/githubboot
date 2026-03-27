package com.abootpatch.app.modules

import android.view.*
import androidx.recyclerview.widget.*
import com.abootpatch.app.core.ABootModule
import com.abootpatch.app.databinding.ItemModuleBinding

class ModuleAdapter(val onToggle:(ABootModule,Boolean)->Unit, val onDelete:(ABootModule)->Unit):ListAdapter<ABootModule,ModuleAdapter.VH>(D){
    inner class VH(val b:ItemModuleBinding):RecyclerView.ViewHolder(b.root){
        fun bind(m:ABootModule){
            b.textModuleName.text=m.name; b.textModuleVersion.text=m.version
            b.textModuleAuthor.text=m.author; b.textModuleDesc.text=m.description
            b.switchModule.setOnCheckedChangeListener(null); b.switchModule.isChecked=m.enabled
            b.switchModule.setOnCheckedChangeListener{_,c->onToggle(m,c)}
            b.btnDelete.setOnClickListener{onDelete(m)}
        }
    }
    override fun onCreateViewHolder(p:ViewGroup,t:Int)=VH(ItemModuleBinding.inflate(LayoutInflater.from(p.context),p,false))
    override fun onBindViewHolder(h:VH,i:Int)=h.bind(getItem(i))
    companion object{ val D=object:DiffUtil.ItemCallback<ABootModule>(){
        override fun areItemsTheSame(a:ABootModule,b:ABootModule)=a.id==b.id
        override fun areContentsTheSame(a:ABootModule,b:ABootModule)=a==b
    }}
}
