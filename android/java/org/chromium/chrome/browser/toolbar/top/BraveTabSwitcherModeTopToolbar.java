/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.top;

import android.content.Context;
import android.util.AttributeSet;
import android.view.View;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.toolbar.NewTabButton;
import org.chromium.chrome.browser.toolbar.bottom.BottomToolbarConfiguration;
import org.chromium.chrome.browser.toolbar.bottom.BottomToolbarVariationManager;
import org.chromium.chrome.browser.toolbar.menu_button.MenuButton;
import org.chromium.components.browser_ui.styles.ChromeColors;

public class BraveTabSwitcherModeTopToolbar extends TabSwitcherModeTopToolbar {
    // To delete in bytecode, members from parent class will be used instead.
    private View mNewTabViewButton;
    private NewTabButton mNewTabImageButton;
    private ToggleTabStackButton mToggleTabStackButton;
    private boolean mShouldShowNewTabVariation;
    private boolean mIsIncognito;

    // Own members.
    private boolean mShouldShowNewTabButton;

    public BraveTabSwitcherModeTopToolbar(Context context, AttributeSet attrs) {
        super(context, attrs);
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
        mShouldShowNewTabButton = !isVisible
                || (BottomToolbarConfiguration.isBottomToolbarEnabled()
                        && !BottomToolbarVariationManager.isNewTabButtonOnBottom());
        updateNewTabButtonVisibility();
        // Show tab switcher button on the top in landscape mode.
        if (BottomToolbarVariationManager.isTabSwitcherOnBottom() && !shouldShowIncognitoToggle()) {
            mToggleTabStackButton.setVisibility(isVisible ? GONE : VISIBLE);
        }
    }

    public int getToolbarColorForCurrentState() {
        // Return primary background color regardless of GridTabSwitcher state. Otherwise toolbar of
        // browsing mode is still visible in tab switching mode with stack layout.
        return ChromeColors.getPrimaryBackgroundColor(getContext(), mIsIncognito);
    }
}
