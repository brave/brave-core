/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.externalnav;

import android.content.SharedPreferences;
import android.content.pm.ResolveInfo;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.chrome.browser.preferences.website.PlayYTVideoInBrowserPreferences;

import java.util.List;

/**
 * Helper class that initializes various tab UserData objects.
 */
public final class BraveEnternalNavigationHandlerHelper {
    private BraveEnternalNavigationHandlerHelper() {}

    private static final String YT_PACKAGE_NAME = "com.google.android.youtube";
    private static final String TAG = "BraveEnternalNavigationHandlerHelper";
    private static final boolean DEBUG = false;

    public static boolean shouldOverrideUrlLoading(String intentPackageName, List<ResolveInfo> resolvingInfos) {
        if (!playYTVideoInBrowserEnabled())
            return true;

        if (intentPackageName != null && intentPackageName.equals(YT_PACKAGE_NAME)) {
            if (DEBUG) Log.i(TAG, "NO_OVERRIDE: YouTube URL for YouTube app");
            return false;
        }

        // Force to open YouTube urls in Brave, override app chooser
        if (ContainsYT(resolvingInfos)) {
            if (DEBUG) Log.i(TAG, "NO_OVERRIDE: YouTube URL for YouTube app (2)");
            return false;
        }
        return true;
    }


    private static boolean playYTVideoInBrowserEnabled() {
        return ContextUtils.getAppSharedPreferences().getBoolean(PlayYTVideoInBrowserPreferences.PLAY_YT_VIDEO_IN_BROWSER_KEY, true);
    }

    /**
     * @return true when resolve infos contain ref to YouTube app
     */
    private static boolean ContainsYT(List<ResolveInfo> resolvingInfos) {
        for (ResolveInfo info : resolvingInfos) {
            String packageName = null;
            if (info.activityInfo != null) {
                packageName = info.activityInfo.packageName;
            } else if (info.serviceInfo != null) {
                packageName = info.serviceInfo.packageName;
            }
            if (YT_PACKAGE_NAME.equalsIgnoreCase(packageName)) {
                return true;
            }
        }
        return false;
    }
}
