/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.share;

import android.content.Context;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.data_sharing.DataSharingTabManager;
import org.chromium.chrome.browser.lifecycle.ActivityLifecycleDispatcher;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.components.browser_ui.bottomsheet.BottomSheetController;

import java.util.function.Supplier;

@NullMarked
public class BraveShareDelegateImpl extends ShareDelegateImpl {
    public BraveShareDelegateImpl(
            Context context,
            @Nullable BottomSheetController controller,
            ActivityLifecycleDispatcher lifecycleDispatcher,
            Supplier<@Nullable Tab> tabProvider,
            Supplier<TabModelSelector> tabModelSelectorProvider,
            Supplier<@Nullable Profile> profileSupplier,
            ShareSheetDelegate delegate,
            boolean isCustomTab,
            @Nullable DataSharingTabManager dataSharingTabManager) {
        super(
                context,
                controller,
                lifecycleDispatcher,
                tabProvider,
                tabModelSelectorProvider,
                profileSupplier,
                delegate,
                isCustomTab,
                dataSharingTabManager);
    }

    @Override
    public boolean isSharingHubEnabled() {
        return super.isSharingHubEnabled()
                && !ChromeSharedPreferences.getInstance()
                        .readBoolean(BravePreferenceKeys.BRAVE_DISABLE_SHARING_HUB, false);
    }
}
