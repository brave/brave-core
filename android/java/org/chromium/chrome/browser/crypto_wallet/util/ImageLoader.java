/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.text.TextUtils;
import android.webkit.URLUtil;
import android.widget.ImageView;

import com.bumptech.glide.Glide;
import com.bumptech.glide.Priority;
import com.bumptech.glide.RequestBuilder;
import com.bumptech.glide.load.engine.DiskCacheStrategy;
import com.bumptech.glide.load.resource.bitmap.CenterInside;
import com.bumptech.glide.load.resource.bitmap.RoundedCorners;
import com.bumptech.glide.load.resource.gif.GifDrawable;

import java.util.Arrays;
import java.util.List;

public class ImageLoader {
    private static final List<String> ANIMATED_LIST = Arrays.asList(".gif");

    public static RequestBuilder<Drawable> createLoadNftRequest(
            String url, Context context, boolean isCircular) {
        RequestBuilder<Drawable> request =
                Glide.with(context).load(url).transform(new CenterInside(), new RoundedCorners(24));
        if (isCircular) {
            request = request.circleCrop();
        }
        return request.priority(Priority.IMMEDIATE).diskCacheStrategy(DiskCacheStrategy.ALL);
    }

    public static boolean isSupported(String url) {
        return isValidImgUrl(url) && !isSvg(url);
    }

    public static boolean isSvg(String url) {
        if (!isValidImgUrl(url)) return false;
        return url.startsWith("data:image/svg") || url.endsWith(".svg");
    }

    private static boolean isValidImgUrl(String url) {
        // Only "data:" or HTTPS URLs.
        return URLUtil.isDataUrl(url) || URLUtil.isHttpsUrl(url);
    }
}
