// Copyright 2019 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.toolbar.bottom;

import android.content.Context;
import android.content.res.ColorStateList;
import android.support.v4.content.ContextCompat;
import android.support.v7.content.res.AppCompatResources;
import android.util.AttributeSet;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ActivityTabProvider;
import org.chromium.chrome.browser.compositor.layouts.OverviewModeBehavior;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.ThemeColorProvider;
import org.chromium.chrome.browser.ThemeColorProvider.ThemeColorObserver;
import org.chromium.chrome.browser.ThemeColorProvider.TintObserver;
import org.chromium.chrome.browser.toolbar.top.ToolbarLayout;
import org.chromium.chrome.browser.flags.FeatureUtilities;
import org.chromium.ui.widget.ChromeImageButton;

/**
 * The bookmarks button.
 */
public class BookmarksButton extends ChromeImageButton implements ThemeColorObserver, TintObserver {
    /** A provider that notifies components when the theme color changes.*/
    private ThemeColorProvider mThemeColorProvider;
    private ColorStateList mCurrentTint;

    /** The bookmark button text label. */
    private TextView mLabel;

    /** The wrapper View that contains the bookmark button and the label. */
    private View mWrapper;

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

    /**
     * @param wrapper The wrapping View of this button.
     */
    public void setWrapperView(ViewGroup wrapper) {
        mWrapper = wrapper;
        mLabel = mWrapper.findViewById(R.id.share_button_label);
        if (FeatureUtilities.isLabeledBottomToolbarEnabled()) mLabel.setVisibility(View.VISIBLE);
    }

    @Override
    public void setOnClickListener(OnClickListener listener) {
        if (mWrapper != null) {
            mWrapper.setOnClickListener(listener);
        } else {
            super.setOnClickListener(listener);
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
    public void onTintChanged(ColorStateList tint, boolean useLight) {
        mCurrentTint = tint;
        ApiCompatibilityUtils.setImageTintList(this, tint);
        if (mLabel != null) mLabel.setTextColor(tint);
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

    public void setActivityTabProvider(ActivityTabProvider activityTabProvider) {
        // sergz: Do nothing here, was added just to avoid extra patching
    }
    
    public void setOverviewModeBehavior(OverviewModeBehavior overviewModeBehavior) {
    }
    
    public void updateButtonEnabledState(Tab tab) {
    }
}
