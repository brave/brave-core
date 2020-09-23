// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.toolbar.bottom;

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

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if (!mTouchEnabled) return true;
        return super.onTouchEvent(event);
    }
}
