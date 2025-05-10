/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.custom_app_icons;

import android.content.ComponentName;
import android.content.Context;
import android.content.pm.PackageManager;

import androidx.annotation.NonNull;

import org.chromium.base.ContextUtils;
import org.chromium.base.shared_preferences.SharedPreferencesManager;
import org.chromium.brave.browser.custom_app_icons.CustomAppIcons.AppIconType;
import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;

@NullMarked
public class CustomAppIconsManager {
    private static final String CURRENT_APP_ICON = "current_app_icon";
    private static final String DEFAULT_ACTIVITY_CLASS = "com.google.android.apps.chrome.Main";

    private static @NonNull CustomAppIconsManager sInstance = new CustomAppIconsManager();

    private SharedPreferencesManager mPrefManager;
    private Context mContext;
    private PackageManager mPackageManager;
    private String mPackageName;

    private CustomAppIconsManager() {
        // Private constructor to prevent instantiation
        mPrefManager = ChromeSharedPreferences.getInstance();
        mContext = ContextUtils.getApplicationContext();
        mPackageManager = mContext.getPackageManager();
        mPackageName = mContext.getPackageName();
    }

    public static CustomAppIconsManager getInstance() {
        return sInstance;
    }

    public void switchIcon(@AppIconType int newAppIconType) {
        disableAllIcons();
        enableIcon(newAppIconType);
        setCurrentIcon(newAppIconType);
    }

    private void disableAllIcons() {
        for (@AppIconType int appIconType : CustomAppIcons.getAllIcons()) {
            setIconState(appIconType, PackageManager.COMPONENT_ENABLED_STATE_DISABLED);
        }
    }

    private void enableIcon(@AppIconType int appIconType) {
        setIconState(appIconType, PackageManager.COMPONENT_ENABLED_STATE_ENABLED);
    }

    private void setIconState(@AppIconType int appIconType, int state) {
        ComponentName component =
                new ComponentName(mPackageName, getIconComponentClassName(appIconType));
        mPackageManager.setComponentEnabledSetting(component, state, PackageManager.DONT_KILL_APP);
    }

    public @AppIconType int getCurrentIcon() {
        return mPrefManager.readInt(CURRENT_APP_ICON, CustomAppIcons.ICON_DEFAULT);
    }

    private void setCurrentIcon(@AppIconType int appIconType) {
        mPrefManager.writeInt(CURRENT_APP_ICON, appIconType);
    }

    private String getIconComponentClassName(@AppIconType int appIconType) {
        if (appIconType == CustomAppIcons.ICON_DEFAULT) {
            return DEFAULT_ACTIVITY_CLASS;
        }
        return mPackageName + CustomAppIcons.getAlias(appIconType);
    }

    public void setPrefManagerForTesting(SharedPreferencesManager testPrefManager) {
        mPrefManager = testPrefManager;
    }
}
