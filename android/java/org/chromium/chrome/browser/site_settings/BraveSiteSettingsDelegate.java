/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.site_settings;

import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;

import org.chromium.base.Callback;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.site_settings.ChromeSiteSettingsDelegate;
import org.chromium.components.embedder_support.browser_context.BrowserContextHandle;
import org.chromium.url.GURL;

public class BraveSiteSettingsDelegate extends ChromeSiteSettingsDelegate {
    public BraveSiteSettingsDelegate(Context context, BrowserContextHandle browserContextHandle) {
        super(context, browserContextHandle);
    }

    @Override
    public void closeButton() {
        Intent intent =
                new Intent(BraveActivity.getChromeTabbedActivity(), ChromeTabbedActivity.class);
        intent.addFlags(Intent.FLAG_ACTIVITY_REORDER_TO_FRONT);
        BraveActivity.getChromeTabbedActivity().startActivity(intent);
    }

    @Override
    public void getFaviconImageForURL(GURL faviconUrl, Callback<Bitmap> callback) {
        if (!faviconUrl.isValid()) {
            callback.onResult(null);
            return;
        }
        super.getFaviconImageForURL(faviconUrl, callback);
    }
}
