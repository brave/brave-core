/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.bottom;

import android.app.Activity;
import android.graphics.Point;
import android.view.Display;

import org.chromium.base.ApplicationStatus;
import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.ContextUtils;
import org.chromium.chrome.browser.app.ChromeActivity;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.toolbar.settings.AddressBarPreference;
import org.chromium.ui.base.DeviceFormFactor;

public class BottomToolbarConfiguration {
    private static final int SMALL_SCREEN_WIDTH = 360;
    private static final int SMALL_SCREEN_HEIGHT = 640;

    public static boolean isBraveBottomControlsEnabled() {
        // We do not use the bottom controls on tablets.
        if (DeviceFormFactor.isNonMultiDisplayContextOnTablet(
                ContextUtils.getApplicationContext())) {
            return false;
        }
        // We do not use the bottom controls with address bar on bottom.
        if (isToolbarBottomAnchored()) {
            return false;
        }
        if (ChromeSharedPreferences.getInstance()
                .readBoolean(BravePreferenceKeys.BRAVE_BOTTOM_TOOLBAR_SET_KEY, false)) {
            return ChromeSharedPreferences.getInstance()
                    .readBoolean(BravePreferenceKeys.BRAVE_BOTTOM_TOOLBAR_ENABLED_KEY, true);
        } else {
            ChromeSharedPreferences.getInstance()
                    .writeBoolean(BravePreferenceKeys.BRAVE_BOTTOM_TOOLBAR_SET_KEY, true);
            boolean enable = true;
            if (isSmallScreen()) {
                enable = false;
            }
            ChromeSharedPreferences.getInstance()
                    .writeBoolean(BravePreferenceKeys.BRAVE_BOTTOM_TOOLBAR_ENABLED_KEY, enable);

            return enable;
        }
    }

    private static boolean isSmallScreen() {
        Activity currentActivity = null;
        for (Activity ref : ApplicationStatus.getRunningActivities()) {
            currentActivity = ref;
            if (!(ref instanceof ChromeActivity)) continue;

            break;
        }
        Display screensize = currentActivity.getWindowManager().getDefaultDisplay();
        Point size = new Point();
        screensize.getSize(size);
        int width = size.x;
        int height = size.y;

        return (width <= SMALL_SCREEN_WIDTH) && (height <= SMALL_SCREEN_HEIGHT);
    }

    public static boolean isToolbarTopAnchored() {
        return AddressBarPreference.isToolbarConfiguredToShowOnTop();
    }

    public static boolean isToolbarBottomAnchored() {
        return !isToolbarTopAnchored();
    }
}
