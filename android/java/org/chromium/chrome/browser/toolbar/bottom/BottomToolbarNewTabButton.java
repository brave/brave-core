// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

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
import org.chromium.chrome.browser.ThemeColorProvider;
import org.chromium.chrome.browser.ThemeColorProvider.ThemeColorObserver;
import org.chromium.chrome.browser.ThemeColorProvider.TintObserver;
import org.chromium.chrome.browser.toolbar.IncognitoStateProvider;
import org.chromium.chrome.browser.toolbar.IncognitoStateProvider.IncognitoStateObserver;
import org.chromium.chrome.browser.toolbar.ToolbarColors;
import org.chromium.ui.widget.ChromeImageButton;

/**
 * The tab switcher new tab button.
 */
class BottomToolbarNewTabButton extends ChromeImageButton
        implements IncognitoStateObserver, ThemeColorObserver, TintObserver {
    /** The gray pill background behind the plus icon. */
    private Drawable mBackground;

    /** The {@link Resources} used to compute the background color. */
    private final Resources mResources;

    /** A provider that notifies when incognito mode is entered or exited. */
    private IncognitoStateProvider mIncognitoStateProvider;

    /** A provider that notifies when the theme color changes.*/
    private ThemeColorProvider mThemeColorProvider;

    public BottomToolbarNewTabButton(Context context, AttributeSet attrs) {
        super(context, attrs);

        mResources = context.getResources();
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
    public void onTintChanged(ColorStateList tint, boolean useLight) {
        ApiCompatibilityUtils.setImageTintList(this, tint);
        updateBackground();
    }

    private void updateBackground() {
        if (mThemeColorProvider == null || mIncognitoStateProvider == null || mBackground == null) {
            return;
        }
        mBackground.setColorFilter(
                ToolbarColors.getTextBoxColorForToolbarBackgroundInNonNativePage(mResources,
                        mThemeColorProvider.getThemeColor(),
                        mThemeColorProvider.useLight()
                                && mIncognitoStateProvider.isIncognitoSelected()),
                PorterDuff.Mode.SRC_IN);
    }
}
