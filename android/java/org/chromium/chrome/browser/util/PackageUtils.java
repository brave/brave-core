/**
 * Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.util;

import android.content.Context;

public class PackageUtils {
    public static boolean isFirstInstall(Context context) {
        try {
            long firstInstallTime = context.getPackageManager()
                                            .getPackageInfo(context.getPackageName(), 0)
                                            .firstInstallTime;
            long lastUpdateTime = context.getPackageManager()
                                          .getPackageInfo(context.getPackageName(), 0)
                                          .lastUpdateTime;
            return firstInstallTime == lastUpdateTime;
        } catch (Exception exc) {
            return false;
        }
    }
}
