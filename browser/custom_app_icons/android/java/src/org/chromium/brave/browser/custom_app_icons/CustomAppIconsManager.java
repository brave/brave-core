/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.custom_app_icons;

import android.content.ComponentName;
import android.content.Context;
import android.content.pm.PackageManager;

import org.chromium.base.ContextUtils;
import org.chromium.base.shared_preferences.SharedPreferencesManager;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;

public class CustomAppIconsManager {
    private static final String CURRENT_APP_ICON = "current_app_icon";
    private static final String DEFAULT_ACTIVITY_CLASS = "com.google.android.apps.chrome.Main";

    private static CustomAppIconsManager instance;

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
        if (instance == null) {
            instance = new CustomAppIconsManager();
        }
        return instance;
    }

    public void switchIcon(CustomAppIconsEnum newIcon) {
        disableAllIcons();
        enableIcon(newIcon);
        setCurrentIcon(newIcon);
    }

    private void disableAllIcons() {
        for (CustomAppIconsEnum icon : CustomAppIconsEnum.values()) {
            setIconState(icon, PackageManager.COMPONENT_ENABLED_STATE_DISABLED);
        }
    }

    private void enableIcon(CustomAppIconsEnum icon) {
        setIconState(icon, PackageManager.COMPONENT_ENABLED_STATE_ENABLED);
    }

    private void setIconState(CustomAppIconsEnum icon, int state) {
        ComponentName component = new ComponentName(mPackageName, getIconComponentClassName(icon));
        mPackageManager.setComponentEnabledSetting(component, state, PackageManager.DONT_KILL_APP);
    }

    public CustomAppIconsEnum getCurrentIcon() {
        String currentIcon =
                mPrefManager.readString(CURRENT_APP_ICON, CustomAppIconsEnum.ICON_DEFAULT.name());
        return CustomAppIconsEnum.valueOf(currentIcon);
    }

    private void setCurrentIcon(CustomAppIconsEnum icon) {
        mPrefManager.writeString(CURRENT_APP_ICON, icon.name());
    }

    private String getIconComponentClassName(CustomAppIconsEnum icon) {
        if (icon == CustomAppIconsEnum.ICON_DEFAULT) {
            return DEFAULT_ACTIVITY_CLASS;
        }
        return mPackageName + icon.getAlias();
    }

    public void setPrefManagerForTesting(SharedPreferencesManager testPrefManager) {
        mPrefManager = testPrefManager;
    }
}
