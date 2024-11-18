/*
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package com.brave.playlist.extension

import android.content.Context
import android.view.WindowManager
import android.widget.PopupWindow
import com.brave.playlist.view.MovableImageButton

fun PopupWindow.addScrimBackground() {
    val rootView = contentView.rootView
    val windowManager: WindowManager =
        rootView.context.getSystemService(Context.WINDOW_SERVICE) as WindowManager
    val params = rootView.layoutParams as WindowManager.LayoutParams
    params.flags = params.flags or WindowManager.LayoutParams.FLAG_DIM_BEHIND
    params.dimAmount = 0.3f
    windowManager.updateViewLayout(rootView, params)
}

fun MovableImageButton.allowMoving(shouldMove: Boolean) {
    if (shouldMove) {
        setOnTouchListener(this)
    }
}
