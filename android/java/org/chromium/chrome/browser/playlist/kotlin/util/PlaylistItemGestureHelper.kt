/*
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package com.brave.playlist.util

import android.content.Context
import android.graphics.Canvas
import android.graphics.Rect
import android.graphics.drawable.ColorDrawable
import android.graphics.drawable.Drawable
import android.view.GestureDetector
import android.view.GestureDetector.SimpleOnGestureListener
import android.view.MotionEvent
import androidx.appcompat.content.res.AppCompatResources
import androidx.recyclerview.widget.ItemTouchHelper.DOWN
import androidx.recyclerview.widget.ItemTouchHelper.END
import androidx.recyclerview.widget.ItemTouchHelper.START
import androidx.recyclerview.widget.ItemTouchHelper.SimpleCallback
import androidx.recyclerview.widget.ItemTouchHelper.UP
import androidx.recyclerview.widget.RecyclerView
import com.brave.playlist.R
import com.brave.playlist.adapter.recyclerview.AbstractRecyclerViewAdapter
import com.brave.playlist.listener.ItemInteractionListener
import kotlin.math.min


class PlaylistItemGestureHelper<VH : AbstractRecyclerViewAdapter.AbstractViewHolder<M>, M : Any>(
    context: Context,
    private val recyclerView: RecyclerView,
    private val adapter: AbstractRecyclerViewAdapter<M, VH>,
    private val itemInteractionListener: ItemInteractionListener
) : SimpleCallback(
    UP or DOWN, START or END
), RecyclerView.OnItemTouchListener {

    private val deleteIcon: Drawable?
    private val shareIcon: Drawable?
    private val deleteIconBg: Drawable
    private val shareIconBg: Drawable
    private val buttonPositions: MutableMap<Int, List<OptionButton>> = mutableMapOf()
    private val gestureDetector: GestureDetector
    private var swipePosition = -1
    private var oldSwipePosition = -1

    private val gestureListener: SimpleOnGestureListener = object : SimpleOnGestureListener() {
        override fun onSingleTapConfirmed(e: MotionEvent): Boolean {
            if (swipePosition != -1) {
                for (button in buttonPositions[swipePosition].orEmpty()) if (button.handleTouch(e)) return true
            }
            return false
        }
    }

    init {
        deleteIcon = AppCompatResources.getDrawable(context, R.drawable.ic_playlist_delete)
        deleteIcon?.setTint(context.getColor(R.color.playlist_progress_bar_tint))
        shareIcon = AppCompatResources.getDrawable(context, R.drawable.ic_share)
        shareIcon?.setTint(context.getColor(R.color.playlist_progress_bar_tint))
        deleteIconBg = ColorDrawable(context.getColor(R.color.swipe_delete))
        shareIconBg = ColorDrawable(context.getColor(R.color.upload_option_bg))
        gestureDetector = GestureDetector(context, gestureListener)
        recyclerView.addOnItemTouchListener(this)
    }

    override fun onMove(
        recyclerView: RecyclerView,
        viewHolder: RecyclerView.ViewHolder,
        target: RecyclerView.ViewHolder
    ): Boolean {
        val fromPosition = viewHolder.bindingAdapterPosition
        val toPosition = target.bindingAdapterPosition
        adapter.swap(fromPosition, toPosition)
        return true
    }

    override fun onSwiped(viewHolder: RecyclerView.ViewHolder, direction: Int) {
        if (direction == START) {
            if (viewHolder.bindingAdapterPosition == oldSwipePosition) oldSwipePosition = -1
            if (viewHolder.bindingAdapterPosition == swipePosition) swipePosition = -1
            buttonPositions.remove(viewHolder.bindingAdapterPosition)
            itemInteractionListener.onItemDelete(viewHolder.layoutPosition)
        } else if (direction == END) oldSwipePosition = swipePosition
    }

    override fun onChildDraw(
        c: Canvas,
        recyclerView: RecyclerView,
        viewHolder: RecyclerView.ViewHolder,
        dX: Float,
        dY: Float,
        actionState: Int,
        isCurrentlyActive: Boolean
    ) {
        var newDX = dX
        if (dX < 0) onSwipeLeft(viewHolder, dX, c)
        else if (dX > 0) {
            newDX = min(onSwipeRight(viewHolder, dX, c), dX)
            swipePosition = viewHolder.bindingAdapterPosition
        } else {
            swipePosition = -1
            resetButtons(c)
        }

        super.onChildDraw(c, recyclerView, viewHolder, newDX, dY, actionState, isCurrentlyActive)
    }

    private fun resetButtons(c: Canvas) {
        resetDrawableBounds(deleteIconBg, c)
        resetDrawableBounds(shareIconBg, c)
    }

    private fun resetDrawableBounds(drawable: Drawable, c: Canvas) {
        drawable.setBounds(0, 0, 0, 0)
        drawable.draw(c)
    }

    private fun onSwipeRight(viewHolder: RecyclerView.ViewHolder, dX: Float, c: Canvas): Float {
        if (shareIcon == null) return 0f

        val itemView = viewHolder.itemView

        val rightBound = itemView.left + dX.toInt()

        if (!buttonPositions.containsKey(viewHolder.bindingAdapterPosition)) buttonPositions[viewHolder.bindingAdapterPosition] =
            instantiateOptions(viewHolder.bindingAdapterPosition)


        buttonPositions[viewHolder.bindingAdapterPosition]?.get(0)?.viewRect = Rect(
            itemView.left, itemView.top, itemView.right, itemView.bottom
        )

        val shareIconMargin = (itemView.height - shareIcon.intrinsicHeight) / 2
        val shareIconTop = itemView.top + (itemView.height - shareIcon.intrinsicHeight) / 2
        val shareIconBottom = shareIconTop + shareIcon.intrinsicHeight
        val shareIconLeft = itemView.left + shareIconMargin
        val shareIconRight = shareIconLeft + shareIcon.intrinsicWidth

        if (rightBound >= shareIconRight) shareIcon.setBounds(
            shareIconLeft, shareIconTop, shareIconRight, shareIconBottom
        )
        else shareIcon.setBounds(0, 0, 0, 0)

        shareIconBg.setBounds(itemView.left, itemView.top, itemView.right, itemView.bottom)

        shareIconBg.draw(c)
        shareIcon.draw(c)

        return (shareIconRight + shareIconMargin).toFloat()
    }

    private fun onSwipeLeft(viewHolder: RecyclerView.ViewHolder, dX: Float, c: Canvas) {
        val itemView = viewHolder.itemView
        if (deleteIcon == null) return

        val iconMargin = (itemView.height - deleteIcon.intrinsicHeight) / 2
        val iconTop = itemView.top + (itemView.height - deleteIcon.intrinsicHeight) / 2
        val iconBottom = iconTop + deleteIcon.intrinsicHeight
        val iconLeft = itemView.right - iconMargin - deleteIcon.intrinsicWidth
        val iconRight = itemView.right - iconMargin
        val leftBound = itemView.right + dX.toInt()

        if (leftBound <= iconLeft) deleteIcon.setBounds(iconLeft, iconTop, iconRight, iconBottom)
        else deleteIcon.setBounds(0, 0, 0, 0)

        deleteIconBg.setBounds(leftBound, itemView.top, itemView.right, itemView.bottom)

        deleteIconBg.draw(c)
        deleteIcon.draw(c)
    }

    override fun onSelectedChanged(
        viewHolder: RecyclerView.ViewHolder?, actionState: Int
    ) {
        super.onSelectedChanged(viewHolder, actionState)


        if (viewHolder is AbstractRecyclerViewAdapter.AbstractViewHolder<*> && !viewHolder.isSelected(
                viewHolder.bindingAdapterPosition
            )
        ) {
            viewHolder.itemView.setBackgroundResource(R.color.playlist_background)
        }
    }

    override fun clearView(
        recyclerView: RecyclerView, viewHolder: RecyclerView.ViewHolder
    ) {
        super.clearView(recyclerView, viewHolder)
        if (viewHolder is AbstractRecyclerViewAdapter.AbstractViewHolder<*> && !viewHolder.isSelected(
                viewHolder.bindingAdapterPosition
            )
        ) viewHolder.itemView.background = null
    }

    override fun getMovementFlags(
        recyclerView: RecyclerView,
        viewHolder: RecyclerView.ViewHolder
    ): Int {
        return makeMovementFlags(UP or DOWN, START or END)
    }

    override fun isLongPressDragEnabled(): Boolean = false

    private fun instantiateOptions(position: Int): List<OptionButton> = listOf(
        OptionButton(position, itemInteractionListener::onShare)
    )

    inner class OptionButton(
        private val adapterPosition: Int, private val click: (position: Int) -> Unit
    ) {
        var viewRect: Rect? = null

        private fun onClick() = click(adapterPosition)

        fun handleTouch(event: MotionEvent): Boolean {
            return viewRect?.contains(event.x.toInt(), event.y.toInt())?.let {
                if (it) {
                    resetSwipedView()
                    onClick()
                }
                it
            } ?: false
        }
    }

    private fun resetSwipedView() {
        adapter.notifyItemChanged(swipePosition)
        swipePosition = -1
        oldSwipePosition = -1
    }

    override fun onInterceptTouchEvent(rv: RecyclerView, e: MotionEvent): Boolean {
        if (e.action == MotionEvent.ACTION_DOWN && swipePosition != -1) {
            recyclerView.findViewHolderForAdapterPosition(swipePosition)?.itemView?.let {
                val rect = Rect(it.left, it.top, it.right, it.bottom)
                if (!rect.contains(e.x.toInt(), e.y.toInt())) resetSwipedView()
            }
        }
        return gestureDetector.onTouchEvent(e)
    }

    override fun onTouchEvent(rv: RecyclerView, e: MotionEvent) {}

    override fun onRequestDisallowInterceptTouchEvent(disallowIntercept: Boolean) {}
}
