/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.shields;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.FrameLayout;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.ui.base.ViewUtils;

/**
 * A FrameLayout that never measures taller than a caller-supplied pixel value. Used to keep the
 * Shields panel, whose content varies with how many advanced-option rows are visible, within
 * whatever vertical space is actually available on screen instead of growing until the system
 * repositions the whole popup to keep it on screen. Children (each wrapped in a ScrollView) scroll
 * internally once the cap is reached.
 */
@NullMarked
public class MaxHeightFrameLayout extends FrameLayout {
    private static final int NOT_SPECIFIED = -1;

    private int mMaxHeightPx = NOT_SPECIFIED;

    public MaxHeightFrameLayout(Context context, @Nullable AttributeSet attrs) {
        super(context, attrs);
    }

    /** Sets the maximum measured height in pixels. Pass a non-positive value to remove the cap. */
    public void setMaxHeightPx(int maxHeightPx) {
        mMaxHeightPx = maxHeightPx > 0 ? maxHeightPx : NOT_SPECIFIED;
        ViewUtils.requestLayout(this, "MaxHeightFrameLayout.setMaxHeightPx");
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        if (mMaxHeightPx != NOT_SPECIFIED) {
            int mode = MeasureSpec.getMode(heightMeasureSpec);
            int size = MeasureSpec.getSize(heightMeasureSpec);
            if (mode != MeasureSpec.EXACTLY || size > mMaxHeightPx) {
                heightMeasureSpec = MeasureSpec.makeMeasureSpec(mMaxHeightPx, MeasureSpec.AT_MOST);
            }
        }
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
    }
}
