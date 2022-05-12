/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.top;

import android.view.ViewStub;

import org.chromium.base.supplier.BooleanSupplier;
import org.chromium.chrome.browser.toolbar.menu_button.MenuButtonCoordinator;

class BraveTabSwitcherModeTTCoordinator extends TabSwitcherModeTTCoordinator {
    private TabSwitcherModeTopToolbar mActiveTabSwitcherToolbar;

    private boolean mIsBottomToolbarVisible;
    private MenuButtonCoordinator mBraveMenuButtonCoordinator;

    BraveTabSwitcherModeTTCoordinator(ViewStub tabSwitcherToolbarStub,
            ViewStub tabSwitcherFullscreenToolbarStub, MenuButtonCoordinator menuButtonCoordinator,
            boolean isGridTabSwitcherEnabled, boolean isTabletGtsPolishEnabled,
            boolean isTabToGtsAnimationEnabled, BooleanSupplier isIncognitoModeEnabledSupplier) {
        super(tabSwitcherToolbarStub, tabSwitcherFullscreenToolbarStub, menuButtonCoordinator,
                isGridTabSwitcherEnabled, isTabletGtsPolishEnabled, isTabToGtsAnimationEnabled,
                isIncognitoModeEnabledSupplier);

        mBraveMenuButtonCoordinator = menuButtonCoordinator;
    }

    @Override
    public void setTabSwitcherMode(boolean inTabSwitcherMode) {
        super.setTabSwitcherMode(inTabSwitcherMode);
        if (inTabSwitcherMode
                && (mActiveTabSwitcherToolbar instanceof BraveTabSwitcherModeTopToolbar)) {
            ((BraveTabSwitcherModeTopToolbar) mActiveTabSwitcherToolbar)
                    .onBottomToolbarVisibilityChanged(mIsBottomToolbarVisible);
        }
        if (mBraveMenuButtonCoordinator != null && mIsBottomToolbarVisible) {
            mBraveMenuButtonCoordinator.setVisibility(!inTabSwitcherMode);
        }
    }

    void onBottomToolbarVisibilityChanged(boolean isVisible) {
        if (mIsBottomToolbarVisible == isVisible) {
            return;
        }
        mIsBottomToolbarVisible = isVisible;
        if (mActiveTabSwitcherToolbar instanceof BraveTabSwitcherModeTopToolbar) {
            ((BraveTabSwitcherModeTopToolbar) mActiveTabSwitcherToolbar)
                    .onBottomToolbarVisibilityChanged(isVisible);
        }
    }
}
