/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.share;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.supplier.Supplier;
import org.chromium.chrome.browser.lifecycle.ActivityLifecycleDispatcher;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.components.browser_ui.bottomsheet.BottomSheetController;

public class BraveShareDelegateImpl extends ShareDelegateImpl {
    public BraveShareDelegateImpl(BottomSheetController controller,
            ActivityLifecycleDispatcher lifecycleDispatcher, Supplier<Tab> tabProvider,
            Supplier<TabModelSelector> tabModelSelectorProvider, Supplier<Profile> profileSupplier,
            ShareSheetDelegate delegate, boolean isCustomTab) {
        super(controller, lifecycleDispatcher, tabProvider, tabModelSelectorProvider,
                profileSupplier, delegate, isCustomTab);
    }

    @Override
    public boolean isSharingHubEnabled() {
        return super.isSharingHubEnabled()
                && !ChromeSharedPreferences.getInstance()
                        .readBoolean(BravePreferenceKeys.BRAVE_DISABLE_SHARING_HUB, false);
    }
}
