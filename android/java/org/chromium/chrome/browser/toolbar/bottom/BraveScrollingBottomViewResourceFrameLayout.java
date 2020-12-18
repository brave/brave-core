/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.bottom;

import android.content.Context;
import android.util.AttributeSet;
import android.view.MotionEvent;

import org.chromium.components.browser_ui.widget.gesture.SwipeGestureListener;
import org.chromium.components.browser_ui.widget.gesture.SwipeGestureListener.SwipeHandler;

public class BraveScrollingBottomViewResourceFrameLayout
        extends ScrollingBottomViewResourceFrameLayout {
    /** A swipe recognizer for handling swipe gestures. */
    private SwipeGestureListener mSwipeGestureListener;

    public BraveScrollingBottomViewResourceFrameLayout(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    /**
     * Set the swipe handler for this view and set {@link #isClickable()} to true to allow motion
     * events to be intercepted by the view itself.
     * @param handler A handler for swipe events on this view.
     */
    public void setSwipeDetector(SwipeHandler handler) {
        mSwipeGestureListener = new SwipeGestureListener(getContext(), handler);

        // TODO(mdjones): This line of code makes it impossible to scroll through the bottom
        // toolbar. If the user accidentally swipes up on this view, the scroll no longer goes
        // through to the web contents. We should figure out how to make this work while also
        // supporting the toolbar swipe behavior.
        setClickable(true);
    }

    @Override
    public boolean onInterceptTouchEvent(MotionEvent event) {
        boolean handledEvent = false;
        if (mSwipeGestureListener != null) handledEvent = mSwipeGestureListener.onTouchEvent(event);
        return handledEvent || super.onInterceptTouchEvent(event);
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        boolean handledEvent = false;
        if (mSwipeGestureListener != null) handledEvent = mSwipeGestureListener.onTouchEvent(event);
        return handledEvent || super.onTouchEvent(event);
    }
}
