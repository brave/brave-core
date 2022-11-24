/**
 * Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.notifications;

import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.provider.Settings;

import androidx.core.content.ContextCompat;

/**
 * This class is for settings permission related checks
 */
public class BravePermissionUtils {
    private static final String APP_PACKAGE = "app_package";
    private static final String APP_UID = "app_uid";

    public static Boolean hasPermission(Context context, String permission) {
        return ContextCompat.checkSelfPermission(context, permission)
                == PackageManager.PERMISSION_GRANTED;
    }

    public static void notificationSettingPage(Context context) {
        Intent intent = new Intent();
        intent.setAction(Settings.ACTION_APP_NOTIFICATION_SETTINGS);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);

        // for Android 5-7
        intent.putExtra(APP_PACKAGE, context.getPackageName());
        intent.putExtra(APP_UID, context.getApplicationInfo().uid);

        // for Android 8 and above
        intent.putExtra(Settings.EXTRA_APP_PACKAGE, context.getPackageName());

        context.startActivity(intent);
    }
}
