/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.util;

import android.graphics.Rect;
import android.util.DisplayMetrics;
import android.view.TouchDelegate;
import android.view.View;
import android.view.ViewTreeObserver.OnGlobalLayoutListener;

public class BraveTouchUtils {
    public static final int MIN_TOUCH_TARGET = 48;

    /**
     * Ensures that the touchable area of [view] equal MIN_TOUCH_TARGET.
     */
    public static void ensureMinTouchTarget(View view) {
        ensureMinTouchTarget(view, MIN_TOUCH_TARGET);
    }

    /**
     * Ensures that the touchable area of [view] equal [minTarget] by expanding the touch area
     * of a view beyond its actual view bounds.
     *
     * This function was taken from Trackr Android App (https://github.com/android/trackr)
     * and adapted to Java. minTarget is in dp instead of px.
     *
     * Copyright (C) 2021 The Android Open Source Project
     *
     * Licensed under the Apache License, Version 2.0 (the "License");
     * you may not use this file except in compliance with the License.
     * You may obtain a copy of the License at
     *
     *      http://www.apache.org/licenses/LICENSE-2.0
     *
     * Unless required by applicable law or agreed to in writing, software
     * distributed under the License is distributed on an "AS IS" BASIS,
     * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
     * See the License for the specific language governing permissions and
     * limitations under the License.
     */
    public static void ensureMinTouchTarget(View view, int minTarget) {
        View parent = (View) view.getParent();
        if (parent != null) {
            parent.getViewTreeObserver().addOnGlobalLayoutListener(new OnGlobalLayoutListener() {
                @Override
                public void onGlobalLayout() {
                    if (view.isShown()) {
                        final Rect bounds = new Rect();
                        view.getHitRect(bounds);

                        DisplayMetrics metrics =
                                view.getContext().getResources().getDisplayMetrics();
                        int height = (int) Math.round(bounds.height() / metrics.density);
                        int width = (int) Math.round(bounds.width() / metrics.density);

                        int extraSpaceStart = 0;
                        int extraSpaceEnd = 0;
                        if (height < minTarget) {
                            extraSpaceStart = (int) Math.floor((minTarget - height) / 2f);
                            extraSpaceEnd = minTarget - height - extraSpaceStart;
                            extraSpaceStart = (int) Math.ceil(extraSpaceStart * metrics.density);
                            extraSpaceEnd = (int) Math.ceil(extraSpaceEnd * metrics.density);
                            bounds.top -= extraSpaceStart;
                            bounds.bottom += extraSpaceEnd;
                        }

                        if (width < minTarget) {
                            extraSpaceStart = (int) Math.floor((minTarget - width) / 2f);
                            extraSpaceEnd = minTarget - width - extraSpaceStart;
                            extraSpaceStart = (int) Math.ceil(extraSpaceStart * metrics.density);
                            extraSpaceEnd = (int) Math.ceil(extraSpaceEnd * metrics.density);
                            bounds.left -= extraSpaceStart;
                            bounds.right += extraSpaceEnd;
                        }

                        ensureTouchDelegateComposite(parent, view);
                        ((TouchDelegateComposite) parent.getTouchDelegate())
                                .addDelegate(bounds, view);
                    }
                }
            });
        }
    }

    private static void ensureTouchDelegateComposite(View parent, View view) {
        TouchDelegate parentTouchDelegate = parent.getTouchDelegate();
        if (parentTouchDelegate == null) {
            parent.setTouchDelegate(new TouchDelegateComposite(new Rect(), view));
        } else if (!(parentTouchDelegate instanceof TouchDelegateComposite)) {
            TouchDelegateComposite touchDelegateComposite =
                    new TouchDelegateComposite(new Rect(), view);
            touchDelegateComposite.addDelegate(parentTouchDelegate);
            parent.setTouchDelegate(touchDelegateComposite);
        }
    }
}
