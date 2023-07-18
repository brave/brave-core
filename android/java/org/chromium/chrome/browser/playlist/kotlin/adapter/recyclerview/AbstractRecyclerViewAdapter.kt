/*
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.playlist.kotlin.adapter.recyclerview

import android.view.View
import androidx.recyclerview.widget.RecyclerView
import java.util.Collections

abstract class AbstractRecyclerViewAdapter<VH : AbstractRecyclerViewAdapter.AbstractViewHolder<M>, M>(
    protected val itemList: MutableList<M>
) :
    RecyclerView.Adapter<VH>(), MutableList<M> {

    abstract class AbstractViewHolder<M>(view: View) : RecyclerView.ViewHolder(view) {
        abstract fun onBind(position: Int, model: M)
        open fun isSelected(position: Int): Boolean = false
    }

    override fun onBindViewHolder(holder: VH, position: Int) =
        holder.onBind(position, itemList[position])

    override fun getItemCount(): Int = itemList.size
    override val size: Int
        get() = itemList.size

    override fun contains(element: M): Boolean = itemList.contains(element)

    override fun containsAll(elements: Collection<M>): Boolean = itemList.containsAll(elements)

    override fun get(index: Int): M = itemList[index]

    override fun indexOf(element: M): Int = itemList.indexOf(element)

    override fun isEmpty(): Boolean = itemList.isEmpty()

    override fun lastIndexOf(element: M): Int = itemList.lastIndexOf(element)

    override fun add(element: M): Boolean {
        val result = itemList.add(element)
        if (result)
            notifyItemInserted(size)
        return result
    }

    override fun add(index: Int, element: M) {
        itemList.add(index, element)
        notifyItemInserted(index)
    }

    override fun addAll(index: Int, elements: Collection<M>): Boolean {
        val result = itemList.addAll(index, elements)
        if (result)
            notifyItemRangeInserted(index, elements.size)
        return result
    }

    override fun addAll(elements: Collection<M>): Boolean {
        val size = size
        val result = itemList.addAll(elements)
        if (result)
            notifyItemRangeInserted(size, elements.size)
        return result
    }

    override fun clear() {
        val size = size
        itemList.clear()
        notifyItemRangeRemoved(0, size)
    }

    override fun remove(element: M): Boolean {
        val index = indexOf(element)
        val result = itemList.remove(element)
        if (result)
            notifyItemRemoved(index)
        return result
    }

    override fun removeAll(elements: Collection<M>): Boolean {
        val result = itemList.removeAll(elements)
        if (result)
            notifyItemRangeRemoved(0, size)
        return result
    }

    override fun removeAt(index: Int): M {
        val item = itemList.removeAt(index)
        notifyItemRemoved(index)
        return item
    }

    override fun retainAll(elements: Collection<M>): Boolean = itemList.retainAll(elements)

    override fun set(index: Int, element: M): M {
        val previousElement = itemList[index]
        itemList[index] = element
        return previousElement
    }

    override fun iterator(): MutableIterator<M> = itemList.iterator()

    override fun listIterator(): MutableListIterator<M> = itemList.listIterator()

    override fun listIterator(index: Int): MutableListIterator<M> = itemList.listIterator(index)

    override fun subList(fromIndex: Int, toIndex: Int): MutableList<M> =
        itemList.subList(fromIndex, toIndex)

    fun swap(fromIndex: Int, toIndex: Int) {
        Collections.swap(itemList, fromIndex, toIndex)
        notifyItemMoved(fromIndex, toIndex)
    }
}
