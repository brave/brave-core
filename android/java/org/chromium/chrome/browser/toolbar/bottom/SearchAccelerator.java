/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.bottom;

import android.content.Context;
import android.content.res.ColorStateList;
import android.graphics.Color;
import android.graphics.PorterDuff;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;

import androidx.core.widget.ImageViewCompat;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.tabmodel.IncognitoStateProvider;
import org.chromium.chrome.browser.tabmodel.IncognitoStateProvider.IncognitoStateObserver;
import org.chromium.chrome.browser.theme.ThemeColorProvider;
import org.chromium.chrome.browser.theme.ThemeColorProvider.ThemeColorObserver;
import org.chromium.chrome.browser.theme.ThemeColorProvider.TintObserver;
import org.chromium.chrome.browser.theme.ThemeUtils;
import org.chromium.chrome.browser.ui.theme.BrandedColorScheme;
import org.chromium.ui.widget.ChromeImageButton;

/** The search accelerator. */
class SearchAccelerator extends ChromeImageButton
        implements ThemeColorObserver, TintObserver, IncognitoStateObserver {
    /** The gray pill background behind the search icon. */
    private final Drawable mBackground;

    /** The {@link Context} used to compute the background color. */
    private final Context mContext;

    /** A provider that notifies components when the theme color changes.*/
    private ThemeColorProvider mThemeColorProvider;

    /** A provider that notifies when incognito mode is entered or exited. */
    private IncognitoStateProvider mIncognitoStateProvider;

    public SearchAccelerator(Context context, AttributeSet attrs) {
        super(context, attrs);

        mContext = context;

        mBackground =
                ApiCompatibilityUtils.getDrawable(
                        mContext.getResources(), R.drawable.home_surface_search_box_background);
        mBackground.mutate();
        setBackground(mBackground);

        setBackgroundColor(Color.TRANSPARENT);
    }

    void setThemeColorProvider(ThemeColorProvider themeColorProvider) {
        mThemeColorProvider = themeColorProvider;
        mThemeColorProvider.addThemeColorObserver(this);
        mThemeColorProvider.addTintObserver(this);
    }

    void setIncognitoStateProvider(IncognitoStateProvider provider) {
        mIncognitoStateProvider = provider;
        mIncognitoStateProvider.addIncognitoStateObserverAndTrigger(this);
    }

    void destroy() {
        if (mThemeColorProvider != null) {
            mThemeColorProvider.removeThemeColorObserver(this);
            mThemeColorProvider.removeTintObserver(this);
            mThemeColorProvider = null;
        }

        if (mIncognitoStateProvider != null) {
            mIncognitoStateProvider.removeObserver(this);
            mIncognitoStateProvider = null;
        }
    }

    @Override
    public void onThemeColorChanged(int color, boolean shouldAnimate) {
        updateBackground();
    }

    @Override
    public void onTintChanged(
            ColorStateList tint,
            ColorStateList activityFocusTint,
            @BrandedColorScheme int brandedColorScheme) {
        ImageViewCompat.setImageTintList(this, tint);
        updateBackground();
    }

    @Override
    public void onIncognitoStateChanged(boolean isIncognito) {
        updateBackground();
    }

    private void updateBackground() {
        if (mThemeColorProvider == null || mIncognitoStateProvider == null) return;

        mBackground.setColorFilter(
                ThemeUtils.getTextBoxColorForToolbarBackgroundInNonNativePage(
                        mContext,
                        mThemeColorProvider.getThemeColor(),
                        mIncognitoStateProvider.isIncognitoSelected(),
                        false /*isCustomTab*/),
                PorterDuff.Mode.SRC_IN);
    }
}
