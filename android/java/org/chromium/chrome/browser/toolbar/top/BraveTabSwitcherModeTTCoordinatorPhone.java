/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.top;

import android.view.ViewStub;

class BraveTabSwitcherModeTTCoordinatorPhone extends TabSwitcherModeTTCoordinatorPhone {
    private TabSwitcherModeTTPhone mTabSwitcherModeToolbar;

    private boolean mIsBottomToolbarVisible;

    BraveTabSwitcherModeTTCoordinatorPhone(ViewStub tabSwitcherToolbarStub) {
        super(tabSwitcherToolbarStub);
    }

    @Override
    public void setTabSwitcherMode(boolean inTabSwitcherMode) {
        super.setTabSwitcherMode(inTabSwitcherMode);
        if (inTabSwitcherMode && (mTabSwitcherModeToolbar instanceof BraveTabSwitcherModeTTPhone)) {
            ((BraveTabSwitcherModeTTPhone) mTabSwitcherModeToolbar)
                    .onBottomToolbarVisibilityChanged(mIsBottomToolbarVisible);
        }
    }

    void onBottomToolbarVisibilityChanged(boolean isVisible) {
        if (mIsBottomToolbarVisible == isVisible) {
            return;
        }
        mIsBottomToolbarVisible = isVisible;
        if (mTabSwitcherModeToolbar instanceof BraveTabSwitcherModeTTPhone) {
            ((BraveTabSwitcherModeTTPhone) mTabSwitcherModeToolbar)
                    .onBottomToolbarVisibilityChanged(isVisible);
        }
    }
}
