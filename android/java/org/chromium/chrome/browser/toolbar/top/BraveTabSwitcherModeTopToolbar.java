/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.top;

import android.content.Context;
import android.util.AttributeSet;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewParent;
import android.widget.FrameLayout;

import org.chromium.chrome.browser.toolbar.NewTabButton;
import org.chromium.chrome.browser.toolbar.R;
import org.chromium.chrome.browser.toolbar.bottom.BottomToolbarConfiguration;
import org.chromium.chrome.browser.toolbar.bottom.BottomToolbarVariationManager;
import org.chromium.components.browser_ui.styles.ChromeColors;
import org.chromium.ui.base.DeviceFormFactor;

public class BraveTabSwitcherModeTopToolbar extends TabSwitcherModeTopToolbar {
    // To delete in bytecode, members from parent class will be used instead.
    private View mNewTabViewButton;
    private NewTabButton mNewTabImageButton;
    private boolean mShouldShowNewTabVariation;
    private boolean mIsIncognito;

    // Own members.
    private boolean mShouldShowNewTabButton;
    private boolean mIsTablet;
    private int mToolbarHeight;

    public BraveTabSwitcherModeTopToolbar(Context context, AttributeSet attrs) {
        super(context, attrs);

        mIsTablet = DeviceFormFactor.isNonMultiDisplayContextOnTablet(context);
        // Tablets don't have bottom toolbar and thus should always show new tab button.
        mShouldShowNewTabButton = mIsTablet;
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);

        if (mIsTablet && mToolbarHeight == 0) {
            mToolbarHeight = getMeasuredHeight();
            maybeUpdateEmptyStateView();
        }
    }

    @Override
    void setTabSwitcherMode(boolean inTabSwitcherMode) {
        super.setTabSwitcherMode(inTabSwitcherMode);

        if (mIsTablet && inTabSwitcherMode && mToolbarHeight != 0) {
            maybeUpdateEmptyStateView();
        }
    }

    protected void updateNewTabButtonVisibility() {
        if (mNewTabViewButton != null) {
            mNewTabViewButton.setVisibility(
                    mShouldShowNewTabVariation && mShouldShowNewTabButton ? VISIBLE : GONE);
        }
        if (mNewTabImageButton != null) {
            mNewTabImageButton.setVisibility(
                    !mShouldShowNewTabVariation && mShouldShowNewTabButton ? VISIBLE : GONE);
        }
    }

    protected boolean shouldShowIncognitoToggle() {
        assert (false);
        return false;
    }

    void onBottomToolbarVisibilityChanged(boolean isVisible) {
        if (mIsTablet) return;

        mShouldShowNewTabButton =
                !isVisible
                        || (BottomToolbarConfiguration.isBottomToolbarEnabled()
                                && !BottomToolbarVariationManager.isNewTabButtonOnBottom());
        updateNewTabButtonVisibility();
    }

    public int getToolbarColorForCurrentState() {
        // Return primary background color regardless of GridTabSwitcher state. Otherwise toolbar of
        // browsing mode is still visible in tab switching mode with stack layout.
        return ChromeColors.getPrimaryBackgroundColor(getContext(), mIsIncognito);
    }

    private void maybeUpdateEmptyStateView() {
        if (!mIsTablet) return;

        // Adjust empty state view top margin to not cover the top toolbar and keep its buttons
        // clickable.
        ViewParent parentView = getParent();
        assert parentView instanceof ViewGroup : "Something has changed in the upstream code!";
        if (parentView instanceof ViewGroup) {
            View emptyView = ((ViewGroup) parentView).findViewById(R.id.empty_state_container);
            if (emptyView != null) {
                FrameLayout.LayoutParams emptyViewParams =
                        (FrameLayout.LayoutParams) emptyView.getLayoutParams();
                emptyViewParams.topMargin = mToolbarHeight;
                emptyView.setLayoutParams(emptyViewParams);
            }
        }
    }
}
