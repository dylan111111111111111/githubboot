package com.abootpatch.app.modules

import android.view.*
import androidx.recyclerview.widget.*
import com.abootpatch.app.core.RootApp
import com.abootpatch.app.databinding.ItemSuperuserBinding

class SuperUserAdapter(val onToggle:(RootApp,Boolean)->Unit):ListAdapter<RootApp,SuperUserAdapter.VH>(D){
    inner class VH(val b:ItemSuperuserBinding):RecyclerView.ViewHolder(b.root){
        fun bind(a:RootApp){
            b.textAppName.text=a.packageName; b.textPackageName.text="uid:${a.uid}"
            b.switchAccess.setOnCheckedChangeListener(null); b.switchAccess.isChecked=a.allowed
            b.switchAccess.setOnCheckedChangeListener{_,c->onToggle(a,c)}
        }
    }
    override fun onCreateViewHolder(p:ViewGroup,t:Int)=VH(ItemSuperuserBinding.inflate(LayoutInflater.from(p.context),p,false))
    override fun onBindViewHolder(h:VH,i:Int)=h.bind(getItem(i))
    companion object{ val D=object:DiffUtil.ItemCallback<RootApp>(){
        override fun areItemsTheSame(a:RootApp,b:RootApp)=a.packageName==b.packageName
        override fun areContentsTheSame(a:RootApp,b:RootApp)=a==b
    }}
}
