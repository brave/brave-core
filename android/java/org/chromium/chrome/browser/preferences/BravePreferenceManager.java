/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.preferences;

import android.app.Activity;
import android.content.SharedPreferences;
import android.graphics.Point;
import android.view.Display;

import org.chromium.base.ApplicationStatus;
import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.chrome.browser.ChromeActivity;

public class BravePreferenceManager {
    public static final String BRAVE_BOTTOM_TOOLBAR_SET_KEY = "brave_bottom_toolbar_set";
    public static final String BRAVE_BOTTOM_TOOLBAR_ENABLED_KEY = "brave_bottom_toolbar_enabled";

    private static final int SMALL_SCREEN_WIDTH = 360;
    private static final int SMALL_SCREEN_HEIGHT = 640;

    public boolean isBottomToolbarEnabled() {
        SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
        if (sharedPreferences.getBoolean(BRAVE_BOTTOM_TOOLBAR_SET_KEY, false)) {
            return sharedPreferences.getBoolean(BRAVE_BOTTOM_TOOLBAR_ENABLED_KEY, true);
        } else {
            ChromePreferenceManager.getInstance().writeBoolean(BRAVE_BOTTOM_TOOLBAR_SET_KEY, true);
            boolean enable = true;
            if (isSmallScreen()) {
                enable = false;
            }
            ChromePreferenceManager.getInstance().writeBoolean(
                    BRAVE_BOTTOM_TOOLBAR_ENABLED_KEY, enable);

            return enable;
        }
    }

    private boolean isSmallScreen() {
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
}
