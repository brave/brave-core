/**
 * Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.util;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.LinearGradient;
import android.graphics.Paint;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffXfermode;
import android.graphics.Shader;
import android.widget.ImageView;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.search_engines.TemplateUrlServiceFactory;
import org.chromium.chrome.browser.ui.favicon.FaviconUtils;
import org.chromium.components.favicon.LargeIconBridge;
import org.chromium.components.favicon.LargeIconBridge.GoogleFaviconServerCallback;
import org.chromium.components.favicon.LargeIconBridge.LargeIconCallback;
import org.chromium.components.search_engines.TemplateUrlService;
import org.chromium.net.NetworkTrafficAnnotationTag;
import org.chromium.url.GURL;

public class ImageUtils {
    public static Bitmap topOffset(Bitmap src, int offsetY) {
        Bitmap outputimage = Bitmap.createBitmap(src.getWidth(), src.getHeight() + offsetY, Bitmap.Config.ARGB_8888);
        Canvas canvas = new Canvas(outputimage);
        canvas.drawBitmap(src, 0, offsetY, null);
        return outputimage;
    }

    public static int calculateInSampleSize(
            BitmapFactory.Options options, int reqWidth, int reqHeight) {
        // Raw height and width of image
        final int height = options.outHeight;
        final int width = options.outWidth;
        int inSampleSize = 1;

        if (height > reqHeight || width > reqWidth) {

            final int halfHeight = height / 2;
            final int halfWidth = width / 2;

            // Calculate the largest inSampleSize value that is a power of 2 and keeps both
            // height and width larger than the requested height and width.
            while ((halfHeight / inSampleSize) >= reqHeight
                    && (halfWidth / inSampleSize) >= reqWidth) {
                inSampleSize *= 2;
            }
        }

        return inSampleSize;
    }

    public static Bitmap addGradient(Bitmap src) {
        Context context = ContextUtils.getApplicationContext();
        int w = src.getWidth();
        int h = src.getHeight();
        Bitmap result = Bitmap.createBitmap(src,0,0,w,h);
        Canvas canvas = new Canvas(result);

        // Top gradient
        int height;

        if(ConfigurationUtils.isLandscape(context)) {
            height = ((2*h)/3);
        } else {
            height = (h/3);
        }

        Paint topPaint = new Paint();
        LinearGradient topShader =
                new LinearGradient(
                        0,
                        0,
                        0,
                        height,
                        context.getColor(R.color.black_alpha_50),
                        Color.TRANSPARENT,
                        Shader.TileMode.CLAMP);
        topPaint.setShader(topShader);
        topPaint.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.DARKEN));
        canvas.drawRect(0,0,w,height,topPaint);

        //Bottom gradient
        Paint bottomPaint = new Paint();
        LinearGradient bottomShader =
                new LinearGradient(
                        0,
                        2 * (h / 3),
                        0,
                        h,
                        Color.TRANSPARENT,
                        context.getColor(R.color.black_alpha_30),
                        Shader.TileMode.CLAMP);
        bottomPaint.setShader(bottomShader);
        bottomPaint.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.DARKEN));
        canvas.drawRect(0,2*(h/3),w,h,bottomPaint);

        return result;
    }

    public static void loadSearchEngineLogo(
            Profile profile, ImageView logoView, String searchKeyword) {
        Context context = ContextUtils.getApplicationContext();
        LargeIconBridge largeIconBridge = new LargeIconBridge(profile);
        TemplateUrlService mTemplateUrlService = TemplateUrlServiceFactory.getForProfile(profile);
        GURL faviconUrl =
                new GURL(mTemplateUrlService.getSearchEngineUrlFromTemplateUrl(searchKeyword));
        // Use a placeholder image while trying to fetch the logo.
        int uiElementSizeInPx =
                context.getResources().getDimensionPixelSize(R.dimen.search_engine_favicon_size);
        logoView.setImageBitmap(
                FaviconUtils.createGenericFaviconBitmap(context, uiElementSizeInPx, null));
        LargeIconCallback onFaviconAvailable =
                (icon, fallbackColor, isFallbackColorDefault, iconType) -> {
                    if (icon != null) {
                        logoView.setImageBitmap(icon);
                        largeIconBridge.destroy();
                    }
                };
        GoogleFaviconServerCallback googleServerCallback =
                (status) -> {
                    // Update the time the icon was last requested to avoid automatic eviction
                    // from cache.
                    largeIconBridge.touchIconFromGoogleServer(faviconUrl);
                    // The search engine logo will be fetched from google servers, so the actual
                    // size of the image is controlled by LargeIconService configuration.
                    // minSizePx=1 is used to accept logo of any size.
                    largeIconBridge.getLargeIconForUrl(
                            faviconUrl,
                            /* minSizePx= */ 1,
                            /* desiredSizePx= */ uiElementSizeInPx,
                            onFaviconAvailable);
                };
        // If the icon already exists in the cache no network request will be made, but the
        // callback will be triggered nonetheless.
        largeIconBridge.getLargeIconOrFallbackStyleFromGoogleServerSkippingLocalCache(
                faviconUrl,
                /* shouldTrimPageUrlPath= */ true,
                NetworkTrafficAnnotationTag.MISSING_TRAFFIC_ANNOTATION,
                googleServerCallback);
    }
}
