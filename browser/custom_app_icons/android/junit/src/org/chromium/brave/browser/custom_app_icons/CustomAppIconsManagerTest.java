/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.custom_app_icons;

import static org.junit.Assert.assertEquals;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.content.pm.PackageManager;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import org.robolectric.annotation.Config;

import org.chromium.base.shared_preferences.SharedPreferencesManager;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.brave.browser.custom_app_icons.CustomAppIcons.AppIconType;

@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class CustomAppIconsManagerTest {

    @Rule public MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private SharedPreferencesManager mPrefManager;

    @Mock private Context mContext;
    @Mock private PackageManager mPackageManager;

    private CustomAppIconsManager mCustomAppIconsManager;

    @Before
    public void setup() {
        mCustomAppIconsManager = CustomAppIconsManager.getInstance();
        mCustomAppIconsManager.setPrefManagerForTesting(mPrefManager);

        when(mContext.getPackageName()).thenReturn("com.example.app");
        when(mContext.getPackageManager()).thenReturn(mPackageManager);
    }

    @Test
    public void testSwitchIcon_DisablesAll_EnablesTarget_SavesPreference() {
        @AppIconType int selectedIcon = CustomAppIcons.ICON_AQUA;
        mCustomAppIconsManager.switchIcon(selectedIcon);
        when(mPrefManager.readInt("current_app_icon", CustomAppIcons.ICON_DEFAULT))
                .thenReturn(selectedIcon);
        assertEquals(selectedIcon, mCustomAppIconsManager.getCurrentIcon());
    }

    @Test
    public void testGetCurrentIcon_ReturnsValueFromPreferences() {
        when(mPrefManager.readInt("current_app_icon", CustomAppIcons.ICON_DEFAULT))
                .thenReturn(CustomAppIcons.ICON_AQUA);

        @AppIconType int currentIcon = mCustomAppIconsManager.getCurrentIcon();
        assertEquals(CustomAppIcons.ICON_AQUA, currentIcon);
    }
}
