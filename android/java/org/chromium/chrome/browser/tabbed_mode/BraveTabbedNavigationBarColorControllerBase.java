/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tabbed_mode;

import android.content.Context;
import android.os.Build;

import androidx.annotation.ColorInt;
import androidx.annotation.RequiresApi;

import org.chromium.base.BraveReflectionUtil;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.toolbar.bottom.BottomToolbarConfiguration;
import org.chromium.chrome.browser.toolbar.menu_button.BraveMenuButtonCoordinator;

/** Base class for the upstream's `TabbedNavigationBarColorController` class. */
@RequiresApi(Build.VERSION_CODES.O_MR1)
class BraveTabbedNavigationBarColorControllerBase {
    /**
     * This variable will be used instead of `TabGroupModelFilter#mContext`, that will be deleted in
     * bytecode.
     */
    protected Context mContext;

    /* Calls from the upstream's private function `TabbedNavigationBarColorController#getNavigationBarColor` will be redirected here via bytecode. */
    @ColorInt
    public int getNavigationBarColor(boolean forceDarkNavigationBar) {
        // Adjust navigation bar color to match the bottom toolbar color when it is visible.
        if (BottomToolbarConfiguration.isBottomToolbarEnabled()
                && BraveMenuButtonCoordinator.isMenuFromBottom()
                && mContext != null) {
            return mContext.getColor(R.color.default_bg_color_baseline);
        }

        // Otherwise just call upstream's method.
        return (int)
                BraveReflectionUtil.invokeMethod(
                        TabbedNavigationBarColorController.class,
                        this,
                        "getNavigationBarColor",
                        boolean.class,
                        forceDarkNavigationBar);
    }
}
