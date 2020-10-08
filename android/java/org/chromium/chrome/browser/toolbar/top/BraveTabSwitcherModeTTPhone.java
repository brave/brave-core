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

public class BraveTabSwitcherModeTTPhone extends TabSwitcherModeTTPhone {
    private View mNewTabViewButton;
    private NewTabButton mNewTabImageButton;
    private ToggleTabStackButton mToggleTabStackButton;
    private MenuButton mMenuButton;
    private boolean mShouldShowNewTabVariation;

    private boolean mShouldShowNewTabButton;

    public BraveTabSwitcherModeTTPhone(Context context, AttributeSet attrs) {
        super(context, attrs);
        mMenuButton = findViewById(R.id.menu_button_wrapper);
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

    private void setMenuButtonVisibility(boolean isButtonVisible) {
        if (mMenuButton != null) {
            mMenuButton.setVisibility(isButtonVisible ? VISIBLE : GONE);
        }
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
        setMenuButtonVisibility(!isVisible
                || (BottomToolbarConfiguration.isBottomToolbarEnabled()
                        && !BottomToolbarVariationManager.isMenuButtonOnBottom()));
    }
}
