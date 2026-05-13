/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import static org.chromium.build.NullUtil.assumeNonNull;

import android.app.Activity;
import android.content.res.Configuration;
import android.view.View;

import org.chromium.build.annotations.NullMarked;

@NullMarked
public class BraveLandscapeHelper {
    public static void applyLandscapeWindowSizing(Activity activity) {
        View content = assumeNonNull(activity.findViewById(android.R.id.content));
        boolean isLandscape =
                activity.getResources().getConfiguration().orientation
                        == Configuration.ORIENTATION_LANDSCAPE;
        int padding = isLandscape ? activity.getResources().getDisplayMetrics().widthPixels / 4 : 0;
        content.setPaddingRelative(
                padding, content.getPaddingTop(), padding, content.getPaddingBottom());
    }
}
