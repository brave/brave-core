/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.bottom;

/**
 * The variation manager helps figure out the current variation and the visibility of buttons on
 * bottom toolbar.
 */
public class BottomToolbarVariationManager {
    /**
     * @return Whether or not new tab button should be visible on the bottom toolbar
     *         in portrait mode in the current variation.
     */
    public static boolean isNewTabButtonOnBottom() {
        return BottomToolbarConfiguration.isBottomToolbarEnabled();
    }

    /**
     * @return Whether or not menu button should be visible on the top toolbar
     *         in portrait mode in the current variation.
     */
    public static boolean isMenuButtonOnBottom() {
        return BottomToolbarConfiguration.isBottomToolbarEnabled();
    }

    /**
     * @return Whether or not bottom toolbar should be visible in overview mode of portrait mode
     *         in the current variation.
     */
    public static boolean shouldBottomToolbarBeVisibleInOverviewMode() {
        return BottomToolbarConfiguration.isBottomToolbarEnabled();
    }

    /**
     * @return Whether or not home button should be visible in top toolbar of portrait mode
     *         in current variation.
     */
    public static boolean isHomeButtonOnBottom() {
        return BottomToolbarConfiguration.isBottomToolbarEnabled();
    }

    /**
     * @return Whether or not tab switcher button should be visible in bottom toolbar
     *         of portrait mode in current variation.
     */
    public static boolean isTabSwitcherOnBottom() {
        return BottomToolbarConfiguration.isBottomToolbarEnabled();
    }

    /**
     * @return Whether or not bookmark button should be visible in bottom toolbar
     *         of portrait mode in current variation.
     */
    public static boolean isBookmarkButtonOnBottom() {
        return BottomToolbarConfiguration.isBottomToolbarEnabled();
    }
}
