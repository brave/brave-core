/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.components.browser_ui.site_settings;


import org.chromium.base.annotations.UsedByReflection;
import org.chromium.base.BraveReflectionUtil;
import org.chromium.components.content_settings.ContentSettingValues;
import org.chromium.components.content_settings.ContentSettingsType;
import org.chromium.components.embedder_support.browser_context.BrowserContextHandle;

@UsedByReflection("Website")
public class BraveWebsite {

    public void setContentSetting(BrowserContextHandle browserContextHandle,
            @ContentSettingsType int type, @ContentSettingValues int value) {
        if (getPermissionInfo(type) != null) {
            getPermissionInfo(type).setContentSetting(browserContextHandle, value);
            return;
        }

        ContentSettingException exception = getContentSettingException(type);
        if (type == ContentSettingsType.AUTOPLAY) {
            if (exception == null) {
                exception = new ContentSettingException(
                        ContentSettingsType.AUTOPLAY, getAddress().getHost(), value, "");
                setContentSettingException(type, exception);
            }
        }
        
        BraveReflectionUtil.InvokeMethod(
                Website.class, this, "setContentSetting",
                BrowserContextHandle.class, browserContextHandle,
                int.class, type, int.class, value);
    }
    
    // placeholders
    public PermissionInfo getPermissionInfo(@ContentSettingsType int type) {
        assert (false);
        return null;
    }

    public ContentSettingException getContentSettingException(@ContentSettingsType int type) {
        assert (false);
        return null;
    }
    
    public void setContentSettingException(
            @ContentSettingsType int type, ContentSettingException exception) {
        assert (false);
    }

    public WebsiteAddress getAddress() {
        assert (false);
        return null;
    }
}
