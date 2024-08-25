/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.components.browser_ui.site_settings;

import androidx.annotation.Nullable;

import org.chromium.base.BraveReflectionUtil;
import org.chromium.components.content_settings.ContentSettingValues;
import org.chromium.components.content_settings.ContentSettingsType;

public class BraveContentSettingsResources extends ContentSettingsResources {
    // Placeholder class
    protected static class ResourceItem {
        ResourceItem(
                int icon,
                int title,
                @ContentSettingValues @Nullable Integer defaultEnabledValue,
                @ContentSettingValues @Nullable Integer defaultDisabledValue,
                int enabledSummary,
                int disabledSummary,
                int summaryOverrideForScreenReader) {
            assert (false);
        }
    }

    protected static ResourceItem getResourceItem(int contentType) {
        switch (contentType) {
            case ContentSettingsType.AUTOPLAY:
                return new ResourceItem(
                        R.drawable.ic_volume_up_grey600_24dp,
                        R.string.autoplay_title,
                        ContentSettingValues.ALLOW,
                        ContentSettingValues.BLOCK,
                        R.string.website_settings_category_autoplay_allowed,
                        0,
                        0);
            case ContentSettingsType.BRAVE_GOOGLE_SIGN_IN:
                return new ResourceItem(
                        R.drawable.ic_person_24dp,
                        R.string.google_sign_in_title,
                        ContentSettingValues.ASK,
                        ContentSettingValues.BLOCK,
                        R.string.website_settings_category_google_sign_in_ask,
                        R.string.website_settings_category_google_sign_in_block,
                        0);
            case ContentSettingsType.BRAVE_LOCALHOST_ACCESS:
                return new ResourceItem(
                        R.drawable.ic_desktop_windows,
                        R.string.localhost_title,
                        ContentSettingValues.ASK,
                        ContentSettingValues.BLOCK,
                        R.string.website_settings_category_localhost_ask,
                        R.string.website_settings_category_localhost_block,
                        0);
        }

        return (ResourceItem)
                BraveReflectionUtil.invokeMethod(
                        ContentSettingsResources.class,
                        null,
                        "getResourceItem",
                        int.class,
                        contentType);
    }
}
