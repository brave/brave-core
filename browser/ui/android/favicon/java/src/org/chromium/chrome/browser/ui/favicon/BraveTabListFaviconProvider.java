/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ui.favicon;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffColorFilter;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;

import androidx.annotation.ColorInt;
import androidx.annotation.VisibleForTesting;
import androidx.appcompat.content.res.AppCompatResources;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.tab_ui.TabCardThemeUtil;
import org.chromium.chrome.browser.tab_ui.TabListFaviconProvider;
import org.chromium.chrome.browser.tab_ui.TabListFaviconProvider.TabFavicon;
import org.chromium.chrome.browser.tab_ui.TabListFaviconProvider.TabWebContentsFaviconDelegate;
import org.chromium.chrome.browser.tab_ui.TabListMode;
import org.chromium.chrome.browser.ui.favicon.brave.R;
import org.chromium.ui.util.ColorUtils;

import java.util.Objects;

/** Provides Brave favicon variants for Chromium tab-list UI. */
@NullMarked
public class BraveTabListFaviconProvider extends TabListFaviconProvider {
    @VisibleForTesting
    static final int BRAVE_NTP_FAVICON_DRAWABLE_ID_OUTLINE =
            R.drawable.ic_social_brave_outline_favicon_fullheight;

    @VisibleForTesting
    static final int BRAVE_NTP_FAVICON_DRAWABLE_ID_FILLED =
            R.drawable.ic_social_brave_carved_favicon_fullheight;

    private final Context mContext;
    private final boolean mIsTabStrip;
    private final int mDefaultFaviconSize;

    public BraveTabListFaviconProvider(
            Context context,
            @TabListMode int tabListMode,
            int faviconCornerRadiusId,
            @Nullable TabWebContentsFaviconDelegate tabWebContentsFaviconDelegate) {
        super(context, tabListMode, faviconCornerRadiusId, tabWebContentsFaviconDelegate);
        mContext = context;
        mIsTabStrip = tabListMode == TabListMode.BOTTOM_STRIP;
        mDefaultFaviconSize =
                context.getResources()
                        .getDimensionPixelSize(
                                org.chromium.chrome.browser.tab_ui.R.dimen.tab_grid_favicon_size);
    }

    @Override
    public TabFavicon getRoundedChromeFavicon(boolean isIncognito) {
        if (mIsTabStrip) {
            return super.getRoundedChromeFavicon(isIncognito);
        }

        int drawableId = getBraveNtpFaviconDrawableId(isIncognito);
        @Nullable Drawable braveNtpSourceDrawable =
                AppCompatResources.getDrawable(mContext, drawableId);
        if (braveNtpSourceDrawable == null) {
            return super.getRoundedChromeFavicon(isIncognito);
        }

        Bitmap braveNtpBitmap =
                getResizedBitmapFromDrawable(braveNtpSourceDrawable, mDefaultFaviconSize);
        @ColorInt
        int defaultIconColor =
                TabCardThemeUtil.getChromeOwnedFaviconTintColor(
                        mContext, isIncognito, /* isSelected= */ false);
        @ColorInt
        int selectedIconColor =
                TabCardThemeUtil.getChromeOwnedFaviconTintColor(
                        mContext, isIncognito, /* isSelected= */ true);

        return new BraveNtpTabFavicon(
                createTintedDrawable(braveNtpBitmap, defaultIconColor),
                createTintedDrawable(braveNtpBitmap, selectedIconColor),
                drawableId,
                defaultIconColor,
                selectedIconColor);
    }

    int getBraveNtpFaviconDrawableId(boolean isIncognito) {
        if (isIncognito || ColorUtils.inNightMode(mContext)) {
            return BRAVE_NTP_FAVICON_DRAWABLE_ID_FILLED;
        }
        return BRAVE_NTP_FAVICON_DRAWABLE_ID_OUTLINE;
    }

    private Drawable createTintedDrawable(Bitmap bitmap, @ColorInt int color) {
        Drawable drawable = new BitmapDrawable(mContext.getResources(), bitmap);
        drawable.setColorFilter(new PorterDuffColorFilter(color, PorterDuff.Mode.SRC_IN));
        return drawable;
    }

    private static Bitmap getResizedBitmapFromDrawable(Drawable drawable, int size) {
        Bitmap bitmap = Bitmap.createBitmap(size, size, Bitmap.Config.ARGB_8888);
        Canvas canvas = new Canvas(bitmap);
        drawable.setBounds(0, 0, size, size);
        drawable.draw(canvas);
        return bitmap;
    }

    private static final class BraveNtpTabFavicon extends TabFavicon {
        private final int mDrawableId;
        private final @ColorInt int mDefaultIconColor;
        private final @ColorInt int mSelectedIconColor;

        BraveNtpTabFavicon(
                Drawable defaultDrawable,
                Drawable selectedDrawable,
                int drawableId,
                @ColorInt int defaultIconColor,
                @ColorInt int selectedIconColor) {
            super(defaultDrawable, selectedDrawable, true);
            mDrawableId = drawableId;
            mDefaultIconColor = defaultIconColor;
            mSelectedIconColor = selectedIconColor;
        }

        @Override
        public int hashCode() {
            return Objects.hash(getClass(), mDrawableId, mDefaultIconColor, mSelectedIconColor);
        }

        @Override
        public boolean equals(@Nullable Object obj) {
            return obj instanceof BraveNtpTabFavicon other
                    && mDrawableId == other.mDrawableId
                    && mDefaultIconColor == other.mDefaultIconColor
                    && mSelectedIconColor == other.mSelectedIconColor;
        }
    }
}
