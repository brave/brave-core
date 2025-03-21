/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

 package org.chromium.brave.browser.custom_app_icons;

import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.graphics.drawable.Drawable;
import android.content.ComponentName;
import android.content.Intent;

public class CustomAppIconsManager {
    public static void switchIcon(Context context, CustomAppIconsEnum customAppIconsEnum) {
        PackageManager packageManager = context.getPackageManager();

        // Disable all existing icons
        for (CustomAppIconsEnum icon : CustomAppIconsEnum.values()) {
            packageManager.setComponentEnabledSetting(
                    new ComponentName(context.getPackageName(), context.getPackageName() + icon.getAlias()),
                    PackageManager.COMPONENT_ENABLED_STATE_DISABLED,
                    PackageManager.DONT_KILL_APP);
        }

        packageManager.setComponentEnabledSetting(
                    new ComponentName(context.getPackageName(), "org.chromium.chrome.browser.ChromeTabbedActivity"),
                    PackageManager.COMPONENT_ENABLED_STATE_DISABLED,
                    PackageManager.DONT_KILL_APP);

        // Enable the selected icon
        packageManager.setComponentEnabledSetting(
                new ComponentName(context.getPackageName(), context.getPackageName() + customAppIconsEnum.getAlias()),
                PackageManager.COMPONENT_ENABLED_STATE_ENABLED,
                PackageManager.DONT_KILL_APP);

                // Intent intent = new Intent(Intent.ACTION_MAIN);
                // intent.addCategory(Intent.CATEGORY_HOME);
                // intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                // context.startActivity(intent);

        // PackageManager pm = context.getPackageManager();

        // pm.setComponentEnabledSetting(
        //         new ComponentName(context.getPackageName(), CustomAppIconsEnum.ICON_DEFAULT.getAlias()),
        //         PackageManager.COMPONENT_ENABLED_STATE_DISABLED,
        //         PackageManager.DONT_KILL_APP);

        // pm.setComponentEnabledSetting(
        //         new ComponentName(context.getPackageName(), CustomAppIconsEnum.ICON_3D.getAlias()),
        //         PackageManager.COMPONENT_ENABLED_STATE_DISABLED,
        //         PackageManager.DONT_KILL_APP);

        // pm.setComponentEnabledSetting(
        //         new ComponentName(context.getPackageName(), CustomAppIconsEnum.ICON_80S.getAlias()),
        //         PackageManager.COMPONENT_ENABLED_STATE_DISABLED,
        //         PackageManager.DONT_KILL_APP);

        // pm.setComponentEnabledSetting(
        //         new ComponentName(context.getPackageName(), CustomAppIconsEnum.ICON_AQUA.getAlias()),
        //         PackageManager.COMPONENT_ENABLED_STATE_DISABLED,
        //         PackageManager.DONT_KILL_APP);

        // pm.setComponentEnabledSetting(
        //         new ComponentName(context.getPackageName(), CustomAppIconsEnum.ICON_BAT.getAlias()),
        //         PackageManager.COMPONENT_ENABLED_STATE_DISABLED,
        //         PackageManager.DONT_KILL_APP);

        // pm.setComponentEnabledSetting(
        //         new ComponentName(context.getPackageName(), CustomAppIconsEnum.ICON_HOLO.getAlias()),
        //         PackageManager.COMPONENT_ENABLED_STATE_DISABLED,
        //         PackageManager.DONT_KILL_APP);

        // pm.setComponentEnabledSetting(
        //         new ComponentName(context.getPackageName(), CustomAppIconsEnum.ICON_NEON.getAlias()),
        //         PackageManager.COMPONENT_ENABLED_STATE_DISABLED,
        //         PackageManager.DONT_KILL_APP);

        // pm.setComponentEnabledSetting(
        //         new ComponentName(context.getPackageName(), CustomAppIconsEnum.ICON_NETSCAPE.getAlias()),
        //         PackageManager.COMPONENT_ENABLED_STATE_DISABLED,
        //         PackageManager.DONT_KILL_APP);

        // pm.setComponentEnabledSetting(
        //         new ComponentName(context.getPackageName(), CustomAppIconsEnum.ICON_POPART.getAlias()),
        //         PackageManager.COMPONENT_ENABLED_STATE_DISABLED,
        //         PackageManager.DONT_KILL_APP);

        // pm.setComponentEnabledSetting(
        //         new ComponentName(context.getPackageName(), CustomAppIconsEnum.ICON_POPARTDARK.getAlias()),
        //         PackageManager.COMPONENT_ENABLED_STATE_DISABLED,
        //         PackageManager.DONT_KILL_APP);

        // pm.setComponentEnabledSetting(
        //         new ComponentName(context.getPackageName(), CustomAppIconsEnum.ICON_SUPERNOVA.getAlias()),
        //         PackageManager.COMPONENT_ENABLED_STATE_DISABLED,
        //         PackageManager.DONT_KILL_APP);

        // pm.setComponentEnabledSetting(
        //         new ComponentName(context.getPackageName(), CustomAppIconsEnum.ICON_TERMINAL.getAlias()),
        //         PackageManager.COMPONENT_ENABLED_STATE_DISABLED,
        //         PackageManager.DONT_KILL_APP);

        // pm.setComponentEnabledSetting(
        //         new ComponentName(context.getPackageName(), CustomAppIconsEnum.ICON_WINDOWS.getAlias()),
        //         PackageManager.COMPONENT_ENABLED_STATE_DISABLED,
        //         PackageManager.DONT_KILL_APP);

        // // Enable the selected one
        // pm.setComponentEnabledSetting(
        //         new ComponentName(context.getPackageName(), customAppIconsEnum.getAlias()),
        //         PackageManager.COMPONENT_ENABLED_STATE_ENABLED,
        //         PackageManager.DONT_KILL_APP);
    }
}

