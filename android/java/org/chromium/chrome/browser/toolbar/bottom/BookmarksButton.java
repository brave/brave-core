/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.bottom;

import android.content.Context;
import android.content.res.ColorStateList;
import android.util.AttributeSet;

import androidx.appcompat.content.res.AppCompatResources;
import androidx.core.content.ContextCompat;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ActivityTabProvider;
import org.chromium.chrome.browser.compositor.layouts.OverviewModeBehavior;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.theme.ThemeColorProvider;
import org.chromium.chrome.browser.theme.ThemeColorProvider.ThemeColorObserver;
import org.chromium.chrome.browser.theme.ThemeColorProvider.TintObserver;
import org.chromium.ui.widget.ChromeImageButton;

/**
 * The bookmarks button.
 */
public class BookmarksButton extends ChromeImageButton implements ThemeColorObserver, TintObserver {
    /** A provider that notifies components when the theme color changes.*/
    private ThemeColorProvider mThemeColorProvider;
    private ColorStateList mCurrentTint;

    public BookmarksButton(Context context, AttributeSet attrs) {
        super(context, attrs);
        setImageDrawable(ContextCompat.getDrawable(context, R.drawable.btn_bookmark));
    }

    public void destroy() {
        if (mThemeColorProvider != null) {
            mThemeColorProvider.removeThemeColorObserver(this);
            mThemeColorProvider.removeTintObserver(this);
            mThemeColorProvider = null;
        }
    }

    public void setThemeColorProvider(ThemeColorProvider themeColorProvider) {
        mThemeColorProvider = themeColorProvider;
        mThemeColorProvider.addThemeColorObserver(this);
        mThemeColorProvider.addTintObserver(this);
    }

    @Override
    public void onThemeColorChanged(int color, boolean shouldAnimate) {
    }

    @Override
    public void onTintChanged(ColorStateList tint, int brandedColorScheme) {
        mCurrentTint = tint;
        ApiCompatibilityUtils.setImageTintList(this, tint);
    }

    public void updateBookmarkButton(boolean isBookmarked, boolean editingAllowed) {
        if (isBookmarked) {
            setImageResource(R.drawable.btn_bookmark_fill);
            ApiCompatibilityUtils.setImageTintList(this,
                    AppCompatResources.getColorStateList(
                            getContext(), R.color.default_icon_color_accent1_tint_list));
            setContentDescription(getContext().getString(R.string.edit_bookmark));
        } else {
            setImageResource(R.drawable.btn_bookmark);
            ApiCompatibilityUtils.setImageTintList(this, mCurrentTint);
            setContentDescription(
                    getContext().getString(R.string.accessibility_menu_bookmark));
        }
        setEnabled(editingAllowed);
    }
    
    public void setOverviewModeBehavior(OverviewModeBehavior overviewModeBehavior) {
    }
    
    public void updateButtonEnabledState(Tab tab) {
    }
}
