/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.bottom;

import android.content.Context;
import android.content.res.ColorStateList;
import android.content.res.Resources;
import android.graphics.PorterDuff;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;

import androidx.annotation.StringRes;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.tabmodel.IncognitoStateProvider;
import org.chromium.chrome.browser.tabmodel.IncognitoStateProvider.IncognitoStateObserver;
import org.chromium.chrome.browser.theme.ThemeColorProvider;
import org.chromium.chrome.browser.theme.ThemeColorProvider.ThemeColorObserver;
import org.chromium.chrome.browser.theme.ThemeColorProvider.TintObserver;
import org.chromium.chrome.browser.theme.ThemeUtils;
import org.chromium.chrome.browser.toolbar.ToolbarColors;
import org.chromium.ui.widget.ChromeImageButton;

/**
 * The tab switcher new tab button.
 */
class BottomToolbarNewTabButton extends ChromeImageButton
        implements IncognitoStateObserver, ThemeColorObserver, TintObserver {
    /** The gray pill background behind the plus icon. */
    private Drawable mBackground;

    /** The {@link Context} used to compute the background color. */
    private final Context mContext;

    /** A provider that notifies when incognito mode is entered or exited. */
    private IncognitoStateProvider mIncognitoStateProvider;

    /** A provider that notifies when the theme color changes.*/
    private ThemeColorProvider mThemeColorProvider;

    public BottomToolbarNewTabButton(Context context, AttributeSet attrs) {
        super(context, attrs);

        mContext = context;
    }

    @Override
    public void setBackground(Drawable background) {
        super.setBackground(background);
        mBackground = background;
    }

    /**
     * Clean up any state when the new tab button is destroyed.
     */
    void destroy() {
        if (mIncognitoStateProvider != null) {
            mIncognitoStateProvider.removeObserver(this);
            mIncognitoStateProvider = null;
        }
        if (mThemeColorProvider != null) {
            mThemeColorProvider.removeThemeColorObserver(this);
            mThemeColorProvider.removeTintObserver(this);
            mThemeColorProvider = null;
        }
    }

    void setIncognitoStateProvider(IncognitoStateProvider incognitoStateProvider) {
        mIncognitoStateProvider = incognitoStateProvider;
        mIncognitoStateProvider.addIncognitoStateObserverAndTrigger(this);
    }

    @Override
    public void onIncognitoStateChanged(boolean isIncognito) {
        @StringRes
        int resId = isIncognito ? R.string.accessibility_toolbar_btn_new_incognito_tab
                                : R.string.accessibility_toolbar_btn_new_tab;
        setContentDescription(getResources().getText(resId));
        updateBackground();
    }

    void setThemeColorProvider(ThemeColorProvider themeColorProvider) {
        mThemeColorProvider = themeColorProvider;
        mThemeColorProvider.addThemeColorObserver(this);
        mThemeColorProvider.addTintObserver(this);
    }

    @Override
    public void onThemeColorChanged(int primaryColor, boolean shouldAnimate) {
        updateBackground();
    }

    @Override
    public void onTintChanged(ColorStateList tint, int brandedColorScheme) {
        ApiCompatibilityUtils.setImageTintList(this, tint);
        updateBackground();
    }

    private void updateBackground() {
        if (mThemeColorProvider == null || mIncognitoStateProvider == null || mBackground == null) {
            return;
        }
        mBackground.setColorFilter(ThemeUtils.getTextBoxColorForToolbarBackgroundInNonNativePage(
                                           mContext, mThemeColorProvider.getThemeColor(),
                                           mIncognitoStateProvider.isIncognitoSelected()),
                PorterDuff.Mode.SRC_IN);
    }
}
