/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.components.browser_ui.site_settings;

import org.chromium.components.browser_ui.site_settings.SiteSettingsCategory.Type;
import org.chromium.components.content_settings.ContentSettingsType;

public class BraveSiteSettingsCategory {
    public static int contentSettingsType(@Type int type) {
        switch (type) {
            case Type.AUTOPLAY:
                return ContentSettingsType.AUTOPLAY;
            case Type.BRAVE_GOOGLE_SIGN_IN:
                return ContentSettingsType.BRAVE_GOOGLE_SIGN_IN;
            case Type.BRAVE_LOCALHOST_ACCESS:
                return ContentSettingsType.BRAVE_LOCALHOST_ACCESS;
            default:
                return SiteSettingsCategory.contentSettingsType(type);
        }
    }

    public static String preferenceKey(@Type int type) {
        switch (type) {
            case Type.AUTOPLAY:
                return "autoplay";
            case Type.BRAVE_GOOGLE_SIGN_IN:
                return "brave_google_sign_in";
            case Type.BRAVE_LOCALHOST_ACCESS:
                return "brave_localhost_access";
            default:
                return SiteSettingsCategory.preferenceKey(type);
        }
    }
}
