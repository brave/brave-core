/*
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.playlist.kotlin.extension

import android.graphics.Color
import android.graphics.LinearGradient
import android.graphics.Shader
import android.graphics.drawable.ShapeDrawable
import android.graphics.drawable.shapes.RoundRectShape
import android.view.View
import android.view.ViewTreeObserver
import org.chromium.chrome.browser.playlist.R
import androidx.core.content.ContextCompat
import org.chromium.chrome.browser.playlist.kotlin.util.PlaylistUtils

fun View.afterMeasured(f: View.() -> Unit) {
    viewTreeObserver.addOnGlobalLayoutListener(object : ViewTreeObserver.OnGlobalLayoutListener {
        override fun onGlobalLayout() {
            if (measuredWidth > 0 && measuredHeight > 0) {
                viewTreeObserver.removeOnGlobalLayoutListener(this)
                f()
            }
        }
    })
}

fun View.showOnboardingGradientBg() {
    val colors = intArrayOf(
        ContextCompat.getColor(context, R.color.playlist_onboarding_gradient_color1),
        ContextCompat.getColor(context, R.color.playlist_onboarding_gradient_color2),
        ContextCompat.getColor(context, R.color.playlist_onboarding_gradient_color3),
        ContextCompat.getColor(context, R.color.playlist_onboarding_gradient_color4),
        ContextCompat.getColor(context, R.color.playlist_onboarding_gradient_color5),
        ContextCompat.getColor(context, R.color.playlist_onboarding_gradient_color6),
        ContextCompat.getColor(context, R.color.playlist_onboarding_gradient_color7),
    )
    val colorPositions = floatArrayOf(0.0142f, 0.1414f, 0.3206f, 0.4944f, 0.6556f, 0.8442f, 1f)

    // apply gradient from left to right of the rectangle along X axis , from top to bottom along Y axis
    val paintShader = LinearGradient(
        0.0f,
        0.0f,
        this.right.toFloat(),
        this.bottom.toFloat(),
        colors,
        colorPositions,// distribution of colors along the length of gradient.
        Shader.TileMode.CLAMP
    )

    val value = PlaylistUtils.dipToPixels(context, 16f)
    val roundedCorners = floatArrayOf(value, value, value, value, value, value, value, value)
    val shapeDrawable = ShapeDrawable(RoundRectShape(roundedCorners, null, null))
    shapeDrawable.paint.shader = paintShader
    this.background = shapeDrawable
}
