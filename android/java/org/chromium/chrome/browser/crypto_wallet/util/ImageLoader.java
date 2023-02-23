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
import com.bumptech.glide.load.DataSource;
import com.bumptech.glide.load.engine.DiskCacheStrategy;
import com.bumptech.glide.load.engine.GlideException;
import com.bumptech.glide.load.resource.bitmap.CenterInside;
import com.bumptech.glide.load.resource.bitmap.RoundedCorners;
import com.bumptech.glide.load.resource.gif.GifDrawable;
import com.bumptech.glide.request.RequestListener;
import com.bumptech.glide.request.target.ImageViewTarget;
import com.bumptech.glide.request.target.Target;

import java.util.Arrays;
import java.util.List;

public class ImageLoader {
    private static final List<String> ANIMATED_LIST = Arrays.asList(".gif");

    public static RequestBuilder<Drawable> createRequest(
            String url, Context context, boolean isCircular, Callback callback) {
        RequestBuilder<Drawable> request =
                Glide.with(context).load(url).transform(new CenterInside(), new RoundedCorners(24));
        if (isCircular) {
            request = request.circleCrop();
        }

        // SVG type is not serializable, so the only disk cache applicable is `DATA`.
        request = request.diskCacheStrategy(
                isSvg(url) ? DiskCacheStrategy.DATA : DiskCacheStrategy.ALL);
        return request.priority(Priority.IMMEDIATE).listener(new RequestListener<Drawable>() {
            @Override
            public boolean onLoadFailed(GlideException glideException, Object model,
                    Target<Drawable> target, boolean isFirstResource) {
                return callback != null ? callback.onLoadFailed() : false;
            }

            @Override
            public boolean onResourceReady(Drawable resource, Object model, Target<Drawable> target,
                    DataSource dataSource, boolean isFirstResource) {
                if (ImageLoader.isSvg(url)) {
                    ImageLoader.setSoftwareLayerType(target);
                }
                return callback != null ? callback.onResourceReady(resource, target) : false;
            }
        });
    }

    /**
     * For SVG images updates the {@link ImageView} to be software rendered, because {@link
     * com.caverock.androidsvg.SVG SVG}/{@link android.graphics.Picture Picture} can't render on a
     * hardware backed {@link android.graphics.Canvas Canvas}.
     */
    public static void setSoftwareLayerType(Target<Drawable> target) {
        ImageView view = ((ImageViewTarget<?>) target).getView();
        view.setLayerType(ImageView.LAYER_TYPE_SOFTWARE, null);
    }

    public static boolean isSupported(String url) {
        return isValidImgUrl(url);
    }

    public static boolean isSvg(String url) {
        if (!isValidImgUrl(url)) return false;
        return url.startsWith("data:image/svg") || url.endsWith(".svg");
    }

    private static boolean isValidImgUrl(String url) {
        // Only "data:" or HTTPS URLs.
        return URLUtil.isDataUrl(url) || URLUtil.isHttpsUrl(url);
    }

    public interface Callback {
        boolean onLoadFailed();
        boolean onResourceReady(Drawable resource, Target<Drawable> target);
    }
}
