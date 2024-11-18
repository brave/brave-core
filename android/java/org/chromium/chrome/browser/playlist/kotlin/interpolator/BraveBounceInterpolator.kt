/*
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.playlist.kotlin.interpolator

import android.view.animation.Interpolator
import kotlin.math.abs
import kotlin.math.cos

class BraveBounceInterpolator(private val bounce: Double = 0.8, private val energy: Double = 1.0) :
    Interpolator {

    override fun getInterpolation(x: Float): Float =
        (1.0 + (-abs(cos(x * 10 * bounce / Math.PI)) * getCurveAdjustment(x))).toFloat()

    private fun getCurveAdjustment(x: Float): Double =
        -(2 * (1 - x) * x * energy + x * x) + 1
}
