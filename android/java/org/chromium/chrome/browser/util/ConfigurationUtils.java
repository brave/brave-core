/**
 * Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.util;

import android.app.Activity;
import android.content.Context;
import android.content.res.Configuration;
import android.util.DisplayMetrics;

import org.chromium.ui.base.DeviceFormFactor;

import java.util.HashMap;

public class ConfigurationUtils {
    public static final String WIDTH = "width";
    public static final String HEIGHT = "height";

    public static boolean isLandscape(Context context) {
        int orientation = context.getResources().getConfiguration().orientation;
        if (orientation == Configuration.ORIENTATION_LANDSCAPE) {
            return true;
        } else {
            return false;
        }
    }

    public static boolean isTablet(Context context) {
        return DeviceFormFactor.isNonMultiDisplayContextOnTablet(context);
    }

    public static HashMap<String, Integer> getDisplayMetrics(Activity activity) {
        DisplayMetrics displayMetrics = new DisplayMetrics();
        activity.getWindowManager().getDefaultDisplay().getMetrics(displayMetrics);
        int deviceWidth = displayMetrics.widthPixels;
        int deviceHeight = displayMetrics.heightPixels;

        HashMap<String, Integer> map = new HashMap<String, Integer>();
        map.put(WIDTH, deviceWidth);
        map.put(HEIGHT, deviceHeight);

        return map;
    }

    public static HashMap<String, Float> getDpDisplayMetrics(Activity activity) {
        DisplayMetrics displayMetrics = new DisplayMetrics();
        activity.getWindowManager().getDefaultDisplay().getMetrics(displayMetrics);
        float dpWidth = displayMetrics.widthPixels / displayMetrics.density;
        float dpHeight = displayMetrics.heightPixels / displayMetrics.density;

        HashMap<String, Float> map = new HashMap<String, Float>();
        map.put(WIDTH, dpWidth);
        map.put(HEIGHT, dpHeight);

        return map;
    }
}
