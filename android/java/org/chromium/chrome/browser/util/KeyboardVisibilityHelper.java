/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.util;

import android.app.Activity;
import android.graphics.Rect;
import android.util.DisplayMetrics;
import android.view.View;
import android.view.ViewTreeObserver;

public class KeyboardVisibilityHelper {
    private final View rootView;
    private final KeyboardVisibilityListener listener;
    private boolean isKeyboardVisible;
    private int keyboardHeight;

    public interface KeyboardVisibilityListener {
        void onKeyboardOpened(int keyboardHeight);

        void onKeyboardClosed();
    }

    public KeyboardVisibilityHelper(Activity activity, KeyboardVisibilityListener listener) {
        this.rootView = activity.findViewById(android.R.id.content);
        this.listener = listener;
        // this.isKeyboardVisible = false;

        rootView.getViewTreeObserver()
                .addOnGlobalLayoutListener(
                        new ViewTreeObserver.OnGlobalLayoutListener() {
                            @Override
                            public void onGlobalLayout() {
                                Rect r = new Rect();
                                rootView.getWindowVisibleDisplayFrame(r);

                                // int screenHeight = getScreenHeight(activity);
                                // int visibleHeight = r.bottom;

                                // int keypadHeight = screenHeight - visibleHeight;
                                // boolean isKeyboardNowVisible = keypadHeight > screenHeight *
                                // 0.15;

                                // if (isKeyboardNowVisible != isKeyboardVisible) {
                                //     isKeyboardVisible = isKeyboardNowVisible;
                                //     if (isKeyboardVisible) {
                                //         listener.onKeyboardOpened(keypadHeight);
                                //     } else {
                                //         listener.onKeyboardClosed();
                                //     }
                                // }
                                int screenHeight = rootView.getRootView().getHeight();
                                int visibleHeight = r.height();
                                int heightDifference = screenHeight - visibleHeight;

                                // Detect if keyboard (including suggestion bar) is visible
                                if (heightDifference > screenHeight * 0.15) {
                                    if (!isKeyboardVisible) {
                                        isKeyboardVisible = true;
                                        keyboardHeight = heightDifference;
                                        // Keyboard is visible, with full height (including
                                        // suggestion bar)
                                        listener.onKeyboardOpened(keyboardHeight);
                                    }
                                } else {
                                    if (isKeyboardVisible) {
                                        isKeyboardVisible = false;
                                        listener.onKeyboardClosed();
                                    }
                                }
                            }
                        });
    }

    private int getScreenHeight(Activity activity) {
        DisplayMetrics displayMetrics = new DisplayMetrics();
        activity.getWindowManager().getDefaultDisplay().getMetrics(displayMetrics);
        return displayMetrics.heightPixels;
    }
}
