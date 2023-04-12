/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.bottom;

import android.annotation.SuppressLint;
import android.content.Context;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.widget.LinearLayout;

import androidx.annotation.Nullable;

/**
 * A linear layout which can intercept touch events to prevent from invoking click listeners on
 * children views.
 */
public class BrowsingModeBottomToolbarLinearLayout extends LinearLayout {
    private boolean mTouchEnabled = true;

    public BrowsingModeBottomToolbarLinearLayout(Context context) {
        super(context);
    }

    public BrowsingModeBottomToolbarLinearLayout(Context context, @Nullable AttributeSet attrs) {
        super(context, attrs);
    }

    public BrowsingModeBottomToolbarLinearLayout(
            Context context, @Nullable AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    /**
     * @param enabled True if all touch events will be intercepted.
     */
    public void setTouchEnabled(boolean enabled) {
        mTouchEnabled = enabled;
    }

    @Override
    public boolean onInterceptTouchEvent(MotionEvent ev) {
        if (!mTouchEnabled) return true;
        return super.onInterceptTouchEvent(ev);
    }

    @SuppressLint("ClickableViewAccessibility")
    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if (!mTouchEnabled) return true;
        return super.onTouchEvent(event);
    }
}
