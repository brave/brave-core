/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.theme;

import android.content.Context;
import android.os.Build;

import org.jni_zero.NativeMethods;

import org.chromium.build.annotations.NullMarked;

/**
 * Passes the browser accent color to native, so native color pipelines (e.g. the WebUI
 * chrome://theme/colors.css) can generate a matching palette.
 */
@NullMarked
public final class BraveAccentColorBridge {
    private BraveAccentColorBridge() {}

    /**
     * Sends the current accent color to native. The accent is the system Material You accent when
     * Brave dynamic colors are enabled, otherwise there is no accent and native uses the default
     * Brave palette. Must be called after native initialization.
     */
    public static void update(Context context) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S
                && BraveDynamicColors.isDynamicColorsEnabled()) {
            // `system_accent1_500` is the mid tone of the system's primary tonal palette; native
            // only uses its hue/chroma as the seed for the generated palette.
            BraveAccentColorBridgeJni.get()
                    .setAccentColor(
                            /* hasAccentColor= */ true,
                            context.getColor(android.R.color.system_accent1_500));
        } else {
            BraveAccentColorBridgeJni.get()
                    .setAccentColor(/* hasAccentColor= */ false, /* accentColor= */ 0);
        }
    }

    @NativeMethods
    interface Natives {
        void setAccentColor(boolean hasAccentColor, int accentColor);
    }
}
