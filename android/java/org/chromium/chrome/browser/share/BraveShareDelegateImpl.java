/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.share;

import org.chromium.base.supplier.Supplier;
import org.chromium.chrome.browser.lifecycle.ActivityLifecycleDispatcher;
import org.chromium.chrome.browser.preferences.BravePreferenceKeys;
import org.chromium.chrome.browser.preferences.SharedPreferencesManager;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.components.browser_ui.bottomsheet.BottomSheetController;

public class BraveShareDelegateImpl extends ShareDelegateImpl {
    public BraveShareDelegateImpl(BottomSheetController controller,
            ActivityLifecycleDispatcher lifecycleDispatcher, Supplier<Tab> tabProvider,
            Supplier<TabModelSelector> tabModelSelectorProvider, ShareSheetDelegate delegate,
            boolean isCustomTab) {
        super(controller, lifecycleDispatcher, tabProvider, tabModelSelectorProvider, delegate,
                isCustomTab);
    }

    @Override
    public boolean isSharingHubEnabled() {
        return super.isSharingHubEnabled()
                && !SharedPreferencesManager.getInstance().readBoolean(
                        BravePreferenceKeys.BRAVE_DISABLE_SHARING_HUB, false);
    }
}
