/*
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.playlist.extension

import com.google.android.material.card.MaterialCardView
import com.google.android.material.shape.CornerFamily
import com.google.android.material.shape.CornerSize
import com.google.android.material.shape.ShapeAppearanceModel

fun MaterialCardView.setTopCornersRounded(dp: Number) {
    val shapeAppearanceModel: ShapeAppearanceModel.Builder = ShapeAppearanceModel().toBuilder()
    val cornerSize = CornerSize { return@CornerSize dp.dpToPx }
    shapeAppearanceModel.setTopLeftCorner(CornerFamily.ROUNDED, cornerSize)
    shapeAppearanceModel.setTopRightCorner(CornerFamily.ROUNDED, cornerSize)
    this.shapeAppearanceModel = shapeAppearanceModel.build()
}
