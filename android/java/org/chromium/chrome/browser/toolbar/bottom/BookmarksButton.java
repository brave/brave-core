// Copyright 2019 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.toolbar.bottom;

import android.content.Context;
import android.content.res.ColorStateList;
import android.util.AttributeSet;

import androidx.appcompat.content.res.AppCompatResources;
import androidx.core.content.ContextCompat;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ActivityTabProvider;
import org.chromium.chrome.browser.ThemeColorProvider;
import org.chromium.chrome.browser.ThemeColorProvider.ThemeColorObserver;
import org.chromium.chrome.browser.ThemeColorProvider.TintObserver;
import org.chromium.chrome.browser.compositor.layouts.OverviewModeBehavior;
import org.chromium.chrome.browser.tab.Tab;

/**
 * The bookmarks button.
 */
public class BookmarksButton extends ShareButton implements ThemeColorObserver, TintObserver {
    /** A provider that notifies components when the theme color changes.*/
    private ThemeColorProvider mThemeColorProvider;
    private ColorStateList mCurrentTint;

    public BookmarksButton(Context context, AttributeSet attrs) {
        super(context, attrs);
        setImageDrawable(ContextCompat.getDrawable(context, R.drawable.btn_bookmark));
    }

    @Override
    public void destroy() {
        if (mThemeColorProvider != null) {
            mThemeColorProvider.removeThemeColorObserver(this);
            mThemeColorProvider.removeTintObserver(this);
            mThemeColorProvider = null;
        }
    }

    @Override
    public void setThemeColorProvider(ThemeColorProvider themeColorProvider) {
        mThemeColorProvider = themeColorProvider;
        mThemeColorProvider.addThemeColorObserver(this);
        mThemeColorProvider.addTintObserver(this);
    }

    @Override
    public void onThemeColorChanged(int color, boolean shouldAnimate) {
    }

    @Override
    public void onTintChanged(ColorStateList tint, boolean useLight) {
        mCurrentTint = tint;
        ApiCompatibilityUtils.setImageTintList(this, tint);
    }

    public void updateBookmarkButton(boolean isBookmarked, boolean editingAllowed) {
        if (isBookmarked) {
            setImageResource(R.drawable.btn_bookmark_fill);
            ApiCompatibilityUtils.setImageTintList(this, AppCompatResources.getColorStateList(getContext(), R.color.blue_mode_tint));
            setContentDescription(getContext().getString(R.string.edit_bookmark));
        } else {
            setImageResource(R.drawable.btn_bookmark);
            ApiCompatibilityUtils.setImageTintList(this, mCurrentTint);
            setContentDescription(
                    getContext().getString(R.string.accessibility_menu_bookmark));
        }
        setEnabled(editingAllowed);
    }

    @Override
    public void setActivityTabProvider(ActivityTabProvider activityTabProvider) {
        // sergz: Do nothing here, was added just to avoid extra patching
    }
    
    @Override
    public void setOverviewModeBehavior(OverviewModeBehavior overviewModeBehavior) {
    }
    
    @Override
    public void updateButtonEnabledState(Tab tab) {
    }
}
