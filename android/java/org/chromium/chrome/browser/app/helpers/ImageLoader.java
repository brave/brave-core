/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.app.helpers;

import android.content.Context;
import android.graphics.Bitmap;

import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.ui.favicon.FaviconHelper;
import org.chromium.url.GURL;

import java.lang.ref.WeakReference;

public class ImageLoader {
    private static FaviconHelper sFaviconHelper;
    private static FaviconHelper.DefaultFaviconHelper sFaviconThemeHelper;

    /**
     * A generic 1-argument callback.
     *
     * @param <T1> The type of the first argument.
     */
    public interface Callback1<T1> {
        /** Call the callback. */
        void call(T1 arg1);
    }

    public static void fetchFavIcon(
            String originSpecUrl,
            boolean useIncognitoNtpIcon,
            WeakReference<Context> context,
            Callback1<Bitmap> callback) {
        try {
            BraveActivity activity = BraveActivity.getBraveActivity();
            FaviconHelper.FaviconImageCallback imageCallback =
                    (bitmap, iconUrl) -> {
                        if (context.get() != null) {
                            if (bitmap == null) {
                                bitmap =
                                        getFaviconThemeHelper()
                                                .getDefaultFaviconBitmap(
                                                        context.get(),
                                                        iconUrl,
                                                        true,
                                                        useIncognitoNtpIcon);
                            }
                            callback.call(bitmap);
                        }
                    };
            // 0 is a max bitmap size for download
            getFaviconHelper()
                    .getLocalFaviconImageForURL(
                            activity.getCurrentProfile(),
                            new GURL(originSpecUrl),
                            0,
                            true,
                            imageCallback);

        } catch (Exception ignored) {
        }
    }

    public static FaviconHelper getFaviconHelper() {
        if (sFaviconHelper == null) {
            sFaviconHelper = new FaviconHelper();
        }
        return sFaviconHelper;
    }

    private static FaviconHelper.DefaultFaviconHelper getFaviconThemeHelper() {
        if (sFaviconThemeHelper == null) {
            sFaviconThemeHelper = new FaviconHelper.DefaultFaviconHelper();
        }
        return sFaviconThemeHelper;
    }
}
