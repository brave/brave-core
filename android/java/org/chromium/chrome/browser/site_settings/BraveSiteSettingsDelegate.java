/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.site_settings;

import android.content.Context;
import android.graphics.drawable.Drawable;

import org.chromium.base.Callback;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.components.browser_ui.site_settings.SiteSettingsCategory;
import org.chromium.url.GURL;

public class BraveSiteSettingsDelegate extends ChromeSiteSettingsDelegate {
    public BraveSiteSettingsDelegate(Context context, Profile profile) {
        super(context, profile);
    }

    @Override
    public void getFaviconImageForURL(GURL faviconUrl, Callback<Drawable> callback) {
        if (!faviconUrl.isValid()) {
            callback.onResult(null);
            return;
        }

        super.getFaviconImageForURL(faviconUrl, callback);
    }

    @Override
    public boolean isCategoryVisible(@SiteSettingsCategory.Type int type) {
        switch (type) {
            case SiteSettingsCategory.Type.STORAGE_ACCESS:
                return false;
            default:
                return super.isCategoryVisible(type);
        }
    }
}
