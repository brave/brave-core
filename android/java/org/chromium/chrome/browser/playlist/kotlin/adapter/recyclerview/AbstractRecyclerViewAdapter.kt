/*
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package com.brave.playlist.adapter.recyclerview

import android.view.View
import androidx.recyclerview.widget.ListAdapter
import androidx.recyclerview.widget.RecyclerView
import com.brave.playlist.PlaylistItemDiffCallback
import java.util.Collections

abstract class AbstractRecyclerViewAdapter<M : Any, VH : AbstractRecyclerViewAdapter.AbstractViewHolder<M>> :
    ListAdapter<M, VH>(PlaylistItemDiffCallback<M>()) {

    abstract class AbstractViewHolder<M>(view: View) : RecyclerView.ViewHolder(view) {
        abstract fun onBind(position: Int, model: M)
        open fun isSelected(position: Int): Boolean = false
    }

    override fun onBindViewHolder(holder: VH, position: Int) =
        holder.onBind(position, currentList[position])

    fun swap(fromIndex: Int, toIndex: Int) {
        val mutableList = currentList.toMutableList()
        Collections.swap(mutableList, fromIndex, toIndex)
        submitList(mutableList)
    }
}
