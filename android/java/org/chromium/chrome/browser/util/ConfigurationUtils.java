/**
 * Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.util;

import android.content.Context;
import android.content.res.Configuration;

public class ConfigurationUtils {
    public static boolean isLandscape(Context context) {
        int orientation = context.getResources().getConfiguration().orientation;
        if (orientation == Configuration.ORIENTATION_LANDSCAPE) {
            return true;
        } else {
            return false;
        }
    }
}