/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.custom_app_icons;

import android.content.ComponentName;
import android.content.Context;
import android.content.pm.PackageManager;

import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;

public class CustomAppIconsManager {
    private static final String CURRENT_APP_ICON = "current_app_icon";

    public static void switchIcon(Context context, CustomAppIconsEnum customAppIconsEnum) {
        PackageManager packageManager = context.getPackageManager();

        // Disable all existing icons
        for (CustomAppIconsEnum icon : CustomAppIconsEnum.values()) {
            packageManager.setComponentEnabledSetting(
                    new ComponentName(context.getPackageName(), getIconClass(context, icon)),
                    PackageManager.COMPONENT_ENABLED_STATE_DISABLED,
                    PackageManager.DONT_KILL_APP);
        }

        // Enable the selected icon
        packageManager.setComponentEnabledSetting(
                new ComponentName(
                        context.getPackageName(), getIconClass(context, customAppIconsEnum)),
                PackageManager.COMPONENT_ENABLED_STATE_ENABLED,
                PackageManager.DONT_KILL_APP);
        setCurrentIcon(customAppIconsEnum);
    }

    public static CustomAppIconsEnum getCurrentIcon(Context context) {
        String currentIcon =
                ChromeSharedPreferences.getInstance()
                        .readString(CURRENT_APP_ICON, CustomAppIconsEnum.ICON_DEFAULT.name());
        return CustomAppIconsEnum.valueOf(currentIcon);
    }

    private static void setCurrentIcon(CustomAppIconsEnum customAppIconsEnum) {
        ChromeSharedPreferences.getInstance()
                .writeString(CURRENT_APP_ICON, customAppIconsEnum.name());
    }

    /**
     * Gets the fully qualified class name for the icon component. Suggested name:
     * getIconComponentClassName
     *
     * @param customAppIconsEnum The enum representing which app icon to use
     * @return The fully qualified class name for the icon component
     */
    private static String getIconClass(Context context, CustomAppIconsEnum customAppIconsEnum) {
        if (customAppIconsEnum == CustomAppIconsEnum.ICON_DEFAULT) {
            return "com.google.android.apps.chrome.Main";
        }
        return context.getPackageName() + customAppIconsEnum.getAlias();
    }
}
