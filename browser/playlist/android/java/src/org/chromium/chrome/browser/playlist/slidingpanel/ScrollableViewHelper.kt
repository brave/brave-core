/*
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.playlist.slidingpanel

import android.view.View
import androidx.recyclerview.widget.RecyclerView

class ScrollableViewHelper {
    fun getScrollableViewScrollPosition(scrollableView: View?, isSlidingUp: Boolean): Int {
        if (scrollableView == null) return 0
        return if (scrollableView is RecyclerView && scrollableView.childCount > 0) {
            val rv: RecyclerView = scrollableView
            val lm: RecyclerView.LayoutManager? = rv.layoutManager
            if (rv.adapter == null) return 0
            if (isSlidingUp) {
                val firstChild: View = rv.getChildAt(0)
                // Approximate the scroll position based on the top child and the first visible item
                lm?.let {
                    rv.getChildLayoutPosition(firstChild) * it.getDecoratedMeasuredHeight(firstChild) - it.getDecoratedTop(
                        firstChild
                    )
                } ?: 0
            } else {
                val lastChild: View = rv.getChildAt(rv.childCount - 1)
                // Approximate the scroll position based on the bottom child and the last visible item
                lm?.let {
                    if ((rv.adapter?.itemCount ?: 0) > 0) {
                        ((rv.adapter?.itemCount
                            ?: 0) - 1) * it.getDecoratedMeasuredHeight(lastChild) + it.getDecoratedBottom(
                            lastChild
                        ) - rv.bottom
                    } else 0
                } ?: 0
            }
        } else {
            0
        }
    }
}
