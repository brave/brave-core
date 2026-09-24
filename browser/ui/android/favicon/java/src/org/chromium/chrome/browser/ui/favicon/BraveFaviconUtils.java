/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ui.favicon;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.drawable.Drawable;

import org.chromium.build.annotations.NullUnmarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.components.browser_ui.widget.RoundedIconGenerator;
import org.chromium.url.GURL;

/** Brave's fix for FaviconUtils to handle null bitmap from icon generator. */
public class BraveFaviconUtils {
    /**
     * Wrapper for {@link FaviconUtils#getIconDrawableWithFilter} that catches NullPointerException
     * when RoundedIconGenerator.generateIconForUrl returns null.
     */
    @NullUnmarked
    public static Drawable getIconDrawableWithFilter(
            @Nullable Bitmap icon,
            @Nullable GURL url,
            RoundedIconGenerator iconGenerator,
            FaviconHelper.DefaultFaviconHelper defaultFaviconHelper,
            Context context,
            int iconSize) {
        try {
            return FaviconUtils.getIconDrawableWithFilter(
                    icon, url, iconGenerator, defaultFaviconHelper, context, iconSize);
        } catch (NullPointerException e) {
            // Fix for crash when generateIconForUrl returns null.
            // Check https://github.com/brave/brave-browser/issues/52458 for more details.
            // Fall back to default favicon.
            return defaultFaviconHelper.getDefaultFaviconDrawable(context, url, true);
        }
    }
}
