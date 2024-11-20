/*
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.playlist.kotlin.view

import android.content.Context
import android.util.AttributeSet
import android.view.MotionEvent
import android.view.View
import android.view.View.OnTouchListener
import android.view.ViewGroup.MarginLayoutParams
import androidx.appcompat.widget.AppCompatImageButton
import kotlin.math.abs
import kotlin.math.max
import kotlin.math.min

class MovableImageButton :
    AppCompatImageButton, OnTouchListener {
    private var downRawX = 0f
    private var downRawY = 0f
    private var dX = 0f
    private var dY = 0f

    constructor(context: Context, attrs: AttributeSet, defStyleAttr: Int) : super(
        context,
        attrs,
        defStyleAttr
    )

    constructor(context: Context) : super(context)

    constructor(context: Context, attrs: AttributeSet) : super(context, attrs)

    override fun onTouch(view: View, motionEvent: MotionEvent): Boolean {
        val layoutParams = view.layoutParams as MarginLayoutParams
        return when (motionEvent.action) {
            MotionEvent.ACTION_DOWN -> {
                downRawX = motionEvent.rawX
                downRawY = motionEvent.rawY
                dX = view.x - downRawX
                dY = view.y - downRawY
                true // Consumed
            }

            MotionEvent.ACTION_MOVE -> {
                val viewWidth = view.width
                val viewHeight = view.height
                val viewParent = view.parent as View
                val parentWidth = viewParent.width
                val parentHeight = viewParent.height
                var newX = motionEvent.rawX + dX

                // Don't allow the FAB past the left hand side of the parent
                newX = max(layoutParams.leftMargin.toFloat(), newX)

                // Don't allow the FAB past the right hand side of the parent
                newX = min((parentWidth - viewWidth - layoutParams.rightMargin).toFloat(), newX)

                var newY = motionEvent.rawY + dY

                // Don't allow the FAB past the top of the parent
                newY = max(layoutParams.topMargin.toFloat(), newY)

                // Don't allow the FAB past the bottom of the parent
                newY = min((parentHeight - viewHeight - layoutParams.bottomMargin).toFloat(), newY)

                //change view position with animation
                view.animate()
                    .x(newX)
                    .y(newY)
                    .setDuration(0)
                    .start()
                true // Consumed
            }

            MotionEvent.ACTION_UP -> {
                val upRawX = motionEvent.rawX
                val upRawY = motionEvent.rawY
                val upDX = upRawX - downRawX
                val upDY = upRawY - downRawY
                if (abs(upDX) < CLICK_DRAG_TOLERANCE && abs(upDY) < CLICK_DRAG_TOLERANCE)  // A click
                    performClick()
                else // A drag
                    true // Consumed
            }

            else -> super.onTouchEvent(motionEvent)
        }
    }

    companion object {
        private const val CLICK_DRAG_TOLERANCE =
            10f // Often, there will be a slight, unintentional, drag when the user taps the Button, so we need to account for this.
    }
}
