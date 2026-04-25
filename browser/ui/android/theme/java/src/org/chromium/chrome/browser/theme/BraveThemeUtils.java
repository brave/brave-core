/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.theme;

import android.content.Context;

import androidx.annotation.ColorInt;

public class BraveThemeUtils {
    public static @ColorInt int getTextBoxColorForToolbarBackgroundInNonNativePage(
            Context context, @ColorInt int color, boolean isIncognito, boolean isCustomTab) {
        if (isIncognito) {
            return context.getColor(R.color.toolbar_text_box_background_incognito);
        }

        return ThemeUtils.getTextBoxColorForToolbarBackgroundInNonNativePage(
                context, color, isIncognito, isCustomTab);
    }
}
