/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.components.browser_ui.site_settings;

import org.chromium.base.BraveReflectionUtil;
import org.chromium.build.annotations.UsedByReflection;
import org.chromium.components.content_settings.ContentSettingValues;
import org.chromium.components.content_settings.ContentSettingsType;
import org.chromium.components.content_settings.ProviderType;
import org.chromium.content_public.browser.BrowserContextHandle;

@UsedByReflection("Website")
public class BraveWebsite {
    public void setContentSetting(
            BrowserContextHandle browserContextHandle,
            @ContentSettingsType.EnumType int type,
            @ContentSettingValues int value) {
        PermissionInfo info =
                (PermissionInfo)
                        BraveReflectionUtil.invokeMethod(
                                Website.class, this, "getPermissionInfo", int.class, type);
        if (info != null) {
            info.setContentSetting(browserContextHandle, value);
            return;
        }

        ContentSettingException exception =
                (ContentSettingException)
                        BraveReflectionUtil.invokeMethod(
                                Website.class, this, "getContentSettingException", int.class, type);
        if (type == ContentSettingsType.AUTOPLAY) {
            if (exception == null) {
                exception =
                        new ContentSettingException(
                                ContentSettingsType.AUTOPLAY,
                                ((WebsiteAddress)
                                                BraveReflectionUtil.invokeMethod(
                                                        Website.class, this, "getAddress"))
                                        .getHost(),
                                value,
                                ProviderType.NONE,
                                false);
                BraveReflectionUtil.invokeMethod(
                        Website.class,
                        this,
                        "setContentSettingException",
                        int.class,
                        type,
                        ContentSettingException.class,
                        exception);
            }
        } else if (type == ContentSettingsType.BRAVE_GOOGLE_SIGN_IN) {
            if (exception == null) {
                exception =
                        new ContentSettingException(
                                ContentSettingsType.BRAVE_GOOGLE_SIGN_IN,
                                ((WebsiteAddress)
                                                BraveReflectionUtil.invokeMethod(
                                                        Website.class, this, "getAddress"))
                                        .getHost(),
                                value,
                                ProviderType.NONE,
                                false);
                BraveReflectionUtil.invokeMethod(
                        Website.class,
                        this,
                        "setContentSettingException",
                        int.class,
                        type,
                        ContentSettingException.class,
                        exception);
            }
        } else if (type == ContentSettingsType.BRAVE_LOCALHOST_ACCESS) {
            if (exception == null) {
                exception =
                        new ContentSettingException(
                                ContentSettingsType.BRAVE_LOCALHOST_ACCESS,
                                ((WebsiteAddress)
                                                BraveReflectionUtil.invokeMethod(
                                                        Website.class, this, "getAddress"))
                                        .getHost(),
                                value,
                                ProviderType.NONE,
                                false);
                BraveReflectionUtil.invokeMethod(
                        Website.class,
                        this,
                        "setContentSettingException",
                        int.class,
                        type,
                        ContentSettingException.class,
                        exception);
            }
        }

        BraveReflectionUtil.invokeMethod(
                Website.class,
                this,
                "setContentSetting",
                BrowserContextHandle.class,
                browserContextHandle,
                int.class,
                type,
                int.class,
                value);
    }
}
