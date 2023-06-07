/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ui.messages.infobar;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffColorFilter;
import android.graphics.drawable.Drawable;

import androidx.annotation.Nullable;
import androidx.core.content.ContextCompat;
import androidx.core.graphics.drawable.DrawableCompat;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.night_mode.GlobalNightModeStateProviderHolder;
import org.chromium.content_public.browser.WebContents;

// This is subclass of SimpleConfirmInfoBarBuilder which allows to pass
// drawableId of vector drawable resource

public class BraveSimpleConfirmInfoBarBuilder extends SimpleConfirmInfoBarBuilder {
    public static void createInfobarWithDrawable(WebContents webContents, SimpleConfirmInfoBarBuilder.Listener listener,
            int infobarTypeIdentifier,
            Context context, int drawableId, String message, String primaryText,
            String secondaryText, String linkText, boolean autoExpire) {

        Bitmap bitmap = null;
        if (drawableId != 0 && context != null) {
            bitmap = convertDrawableToBitmap(context, drawableId);
        }

        SimpleConfirmInfoBarBuilderJni.get().create(webContents, infobarTypeIdentifier, bitmap,
            message, primaryText, secondaryText, linkText, autoExpire, listener);
    }

    @Nullable
    public static Bitmap convertDrawableToBitmap(Context context, int drawableId) {
        Drawable drawable = ContextCompat.getDrawable(context, drawableId);
        if (drawable == null) {
            return null;
        }
        drawable = DrawableCompat.wrap(drawable);

        Bitmap bitmap = Bitmap.createBitmap(drawable.getIntrinsicWidth(),
                drawable.getIntrinsicHeight(), Bitmap.Config.ARGB_8888);
        Canvas canvas = new Canvas(bitmap);
        drawable.setBounds(0, 0, canvas.getWidth(), canvas.getHeight());

        int icon_color = GlobalNightModeStateProviderHolder.getInstance().isInNightMode()
                ? ContextCompat.getColor(context, R.color.brave_informer_dark_theme_icon_color)
                : ContextCompat.getColor(context, R.color.brave_informer_light_theme_icon_color);
        drawable.setColorFilter(new PorterDuffColorFilter(icon_color, PorterDuff.Mode.SRC_ATOP));
        drawable.draw(canvas);
        return bitmap;
    }
}
