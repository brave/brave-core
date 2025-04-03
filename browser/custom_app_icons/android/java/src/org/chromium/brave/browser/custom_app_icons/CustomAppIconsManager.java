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
    private static final String DEFAULT_ACTIVITY_CLASS = "com.google.android.apps.chrome.Main";

    public static void switchIcon(Context context, CustomAppIconsEnum newIcon) {
        disableAllIcons(context);
        enableIcon(context, newIcon);
        setCurrentIcon(newIcon);
    }

    private static void disableAllIcons(Context context) {
        PackageManager packageManager = context.getPackageManager();
        for (CustomAppIconsEnum icon : CustomAppIconsEnum.values()) {
            setIconState(
                    context, packageManager, icon, PackageManager.COMPONENT_ENABLED_STATE_DISABLED);
        }
    }

    private static void enableIcon(Context context, CustomAppIconsEnum icon) {
        setIconState(
                context,
                context.getPackageManager(),
                icon,
                PackageManager.COMPONENT_ENABLED_STATE_ENABLED);
    }

    private static void setIconState(
            Context context, PackageManager packageManager, CustomAppIconsEnum icon, int state) {
        ComponentName component =
                new ComponentName(
                        context.getPackageName(), getIconComponentClassName(context, icon));
        packageManager.setComponentEnabledSetting(component, state, PackageManager.DONT_KILL_APP);
    }

    public static CustomAppIconsEnum getCurrentIcon(Context context) {
        String currentIcon =
                ChromeSharedPreferences.getInstance()
                        .readString(CURRENT_APP_ICON, CustomAppIconsEnum.ICON_DEFAULT.name());
        return CustomAppIconsEnum.valueOf(currentIcon);
    }

    private static void setCurrentIcon(CustomAppIconsEnum icon) {
        ChromeSharedPreferences.getInstance().writeString(CURRENT_APP_ICON, icon.name());
    }

    /**
     * Gets the fully qualified class name for the icon component.
     *
     * @param context The application context
     * @param icon The enum representing which app icon to use
     * @return The fully qualified class name for the icon component
     */
    private static String getIconComponentClassName(Context context, CustomAppIconsEnum icon) {
        if (icon == CustomAppIconsEnum.ICON_DEFAULT) {
            return DEFAULT_ACTIVITY_CLASS;
        }
        return context.getPackageName() + icon.getAlias();
    }
}
