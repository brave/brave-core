// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.toolbar.bottom;

import androidx.annotation.StringDef;
import androidx.annotation.VisibleForTesting;

import org.chromium.chrome.browser.flags.CachedFeatureFlags;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.flags.StringCachedFieldTrialParameter;
import org.chromium.chrome.browser.incognito.IncognitoUtils;
import org.chromium.chrome.browser.preferences.ChromePreferenceKeys;
import org.chromium.chrome.browser.tasks.tab_management.TabUiFeatureUtilities;
import org.chromium.chrome.features.start_surface.StartSurfaceConfiguration;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * The variation manager helps figure out the current variation and the visibility of buttons on
 * bottom toolbar. Every operation related to the variation, e.g. getting variation value, should be
 * through {@link BottomToolbarVariationManager} rather than calling {@link CachedFeatureFlags}.
 */
public class BottomToolbarVariationManager {
    /**
     * @return Whether or not share button should be visible on the top toolbar in portrait mode
     *         in the current variation.
     */
    public static boolean isShareButtonOnBottom() {
        return false;
    }

    /**
     * @return Whether or not new tab button should be visible on the bottom toolbar
     *         in portrait mode in the current variation.
     */
    public static boolean isNewTabButtonOnBottom() {
        return BraveBottomToolbarConfiguration.isBottomToolbarEnabled();
    }

    /**
     * @return Whether or not menu button should be visible on the top toolbar
     *         in portrait mode in the current variation.
     */
    public static boolean isMenuButtonOnBottom() {
        return BraveBottomToolbarConfiguration.isBottomToolbarEnabled();
    }

    /**
     * @return Whether or not bottom toolbar should be visible in overview mode of portrait mode
     *         in the current variation.
     */
    public static boolean shouldBottomToolbarBeVisibleInOverviewMode() {
        return BraveBottomToolbarConfiguration.isBottomToolbarEnabled();
    }

    /**
     * @return Whether or not home button should be visible in top toolbar of portrait mode
     *         in current variation.
     */
    public static boolean isHomeButtonOnBottom() {
        return BraveBottomToolbarConfiguration.isBottomToolbarEnabled();
    }

    /**
     * @return Whether or not tab switcher button should be visible in bottom toolbar
     *         of portrait mode in current variation.
     */
    public static boolean isTabSwitcherOnBottom() {
        return BraveBottomToolbarConfiguration.isBottomToolbarEnabled();
    }

    /**
     * @return Whether or not bookmark button should be visible in bottom toolbar
     *         of portrait mode in current variation.
     */
    public static boolean isBookmarkButtonOnBottom() {
        return BraveBottomToolbarConfiguration.isBottomToolbarEnabled();
    }
}
