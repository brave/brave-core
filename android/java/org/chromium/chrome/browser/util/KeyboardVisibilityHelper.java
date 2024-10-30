/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.util;

import android.app.Activity;
import android.graphics.Rect;
import android.view.View;
import android.view.ViewTreeObserver;

public class KeyboardVisibilityHelper {
    private final View rootView;
    private final KeyboardVisibilityListener listener;
    private boolean isKeyboardVisible;

    public interface KeyboardVisibilityListener {
        void onKeyboardOpened(int keyboardHeight);

        void onKeyboardClosed();
    }

    public KeyboardVisibilityHelper(Activity activity, KeyboardVisibilityListener listener) {
        this.rootView = activity.findViewById(android.R.id.content);
        this.listener = listener;

        rootView.getViewTreeObserver()
                .addOnGlobalLayoutListener(
                        new ViewTreeObserver.OnGlobalLayoutListener() {
                            @Override
                            public void onGlobalLayout() {
                                Rect r = new Rect();
                                rootView.getWindowVisibleDisplayFrame(r);
                                int screenHeight = rootView.getRootView().getHeight();
                                int visibleHeight = r.height();
                                int heightDifference = screenHeight - visibleHeight;

                                boolean keyboardVisible = heightDifference > screenHeight * 0.15;
                                if (keyboardVisible != isKeyboardVisible) {
                                    isKeyboardVisible = keyboardVisible;
                                    // rootView.setPadding(0, 0, 0, keyboardVisible ?
                                    // heightDifference : 0);
                                    if (keyboardVisible) {
                                        listener.onKeyboardOpened(heightDifference);
                                    } else {
                                        listener.onKeyboardClosed();
                                    }
                                }
                            }
                        });
    }
}
