/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.bottom;

import android.annotation.SuppressLint;
import android.content.Context;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;

import org.chromium.base.CallbackController;
import org.chromium.base.supplier.Supplier;
import org.chromium.chrome.R;
import org.chromium.components.browser_ui.widget.gesture.SwipeGestureListener;
import org.chromium.components.browser_ui.widget.gesture.SwipeGestureListener.SwipeHandler;

public class BraveScrollingBottomViewResourceFrameLayout
        extends ScrollingBottomViewResourceFrameLayout {
    /** A swipe recognizer for handling swipe gestures. */
    private SwipeGestureListener mSwipeGestureListener;
    private Supplier<BottomControlsCoordinator> mBottomControlsCoordinatorSupplier;
    private final CallbackController mCallbackController;
    private boolean mIsBottomToolbarVisible;
    private boolean mIsTabGroupUiVisible;

    public BraveScrollingBottomViewResourceFrameLayout(Context context, AttributeSet attrs) {
        super(context, attrs);
        mCallbackController = new CallbackController();
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

    @SuppressLint("ClickableViewAccessibility")
    @Override
    public boolean onTouchEvent(MotionEvent event) {
        boolean handledEvent = false;
        if (mSwipeGestureListener != null) handledEvent = mSwipeGestureListener.onTouchEvent(event);
        return handledEvent || super.onTouchEvent(event);
    }

    private void updateBottomControlsVisibility() {
        if (braveBottomControlsCoordinator() != null) {
            View bottomToolbar = findViewById(R.id.bottom_toolbar);
            assert (bottomToolbar != null);
            if (bottomToolbar != null) {
                bottomToolbar.setVisibility(mIsBottomToolbarVisible ? View.VISIBLE : View.GONE);
            }
            View bottomContainerSlot = findViewById(R.id.bottom_container_slot);
            assert (bottomContainerSlot != null);
            if (bottomContainerSlot != null) {
                bottomContainerSlot.setVisibility(mIsTabGroupUiVisible ? View.VISIBLE : View.GONE);
            }
        }
    }

    public void setBottomControlsCoordinatorSupplier(
            Supplier<BottomControlsCoordinator> bottomControlsCoordinatorSupplier) {
        if (mBottomControlsCoordinatorSupplier != null) {
            assert false : "BottomControlsCoordinatorSupplier should be set once.";
            return;
        }
        mBottomControlsCoordinatorSupplier = bottomControlsCoordinatorSupplier;
        braveBottomControlsCoordinator().getBottomToolbarVisibleSupplier().addObserver(
                mCallbackController.makeCancelable((visible) -> {
                    mIsBottomToolbarVisible = visible;
                    updateBottomControlsVisibility();
                }));
        braveBottomControlsCoordinator().getTabGroupUiVisibleSupplier().addObserver(
                mCallbackController.makeCancelable((visible) -> {
                    mIsTabGroupUiVisible = visible;
                    updateBottomControlsVisibility();
                }));
    }

    private BraveBottomControlsCoordinator braveBottomControlsCoordinator() {
        if (mBottomControlsCoordinatorSupplier != null
                && mBottomControlsCoordinatorSupplier.get()
                                instanceof BraveBottomControlsCoordinator) {
            return (BraveBottomControlsCoordinator) mBottomControlsCoordinatorSupplier.get();
        }
        return null;
    }

    void destroy() {
        mCallbackController.destroy();
    }
}
