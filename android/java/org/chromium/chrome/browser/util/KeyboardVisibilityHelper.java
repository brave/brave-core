/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.util;

import android.graphics.Rect;
import android.view.View;
import android.view.ViewTreeObserver;
import org.chromium.base.Log;

public class KeyboardVisibilityHelper {
    private boolean mIsKeyboardVisible;
    private View mRootView;
    private KeyboardVisibilityListener mKeyboardVisibilityListener;

    public interface KeyboardVisibilityListener {
        void onKeyboardOpened(int keyboardHeight);

        void onKeyboardClosed();
    }

    public KeyboardVisibilityHelper(
            View rootView, KeyboardVisibilityListener keyboardVisibilityListener) {
        mRootView = rootView;
        mKeyboardVisibilityListener = keyboardVisibilityListener;
    }

    private ViewTreeObserver.OnGlobalLayoutListener mListener =
            new ViewTreeObserver.OnGlobalLayoutListener() {
                @Override
                public void onGlobalLayout() {
                    if (mRootView == null) {
                        Log.e("quick_search", "mListener 1");
                        return;
                    }
                    Rect r = new Rect();
                    mRootView.getWindowVisibleDisplayFrame(r);
                    int screenHeight = mRootView.getRootView().getHeight();
                    int visibleHeight = r.bottom;
                    int heightDifference = screenHeight - visibleHeight;

                    boolean keyboardVisible = heightDifference > screenHeight * 0.15;
                    if (keyboardVisible != mIsKeyboardVisible) {
                        mIsKeyboardVisible = keyboardVisible;
                        if (keyboardVisible) {
                            // mKeyboardVisibilityListener.onKeyboardOpened(heightDifference);
                        } else {
                            mKeyboardVisibilityListener.onKeyboardClosed();
                        }
                    }
                }
            };

    public void addListener() {
        Log.e("quick_search", "addListener 1");
        if (mRootView != null) {
            Log.e("quick_search", "addListener 2");
            mRootView.getViewTreeObserver().addOnGlobalLayoutListener(mListener);
        }
    }

    public void removeListener() {
        Log.e("quick_search", "removeListener 1");
        if (mRootView != null) {
            Log.e("quick_search", "removeListener 2");
            mRootView.getViewTreeObserver().removeOnGlobalLayoutListener(mListener);
        }
    }
}
