/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.multiwindow;

import static org.chromium.build.NullUtil.assertNonNull;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.tab.Tab;

import java.util.List;

@NullMarked
public class BraveMultiInstanceOrchestratorImpl extends MultiInstanceOrchestratorImpl {
    private @Nullable BraveMultiInstanceManagerApi31 mBraveMultiInstaceManager;

    BraveMultiInstanceOrchestratorImpl(TabReparentingDelegate tabReparentingDelegate) {
        super(tabReparentingDelegate);
    }

    void setBraveMultiInstanceManager(BraveMultiInstanceManagerApi31 braveMultiInstaceManager) {
        mBraveMultiInstaceManager = braveMultiInstaceManager;
    }

    @Override
    public void moveTabsToWindowByIdChecked(
            int destWindowId,
            List<Tab> tabs,
            int destTabIndex,
            int destGroupTabId,
            boolean bringToFront) {
        super.moveTabsToWindowByIdChecked(
                destWindowId, tabs, destTabIndex, destGroupTabId, bringToFront);

        assertNonNull(mBraveMultiInstaceManager);
        mBraveMultiInstaceManager.moveTabsToWindowByIdChecked(
                destWindowId, tabs, destTabIndex, destGroupTabId, bringToFront);
    }
}
