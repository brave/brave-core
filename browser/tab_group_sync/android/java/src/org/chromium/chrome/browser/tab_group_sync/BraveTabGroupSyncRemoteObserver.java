/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tab_group_sync;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.Callback;
import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.tabmodel.TabModel;
import org.chromium.components.prefs.PrefService;
import org.chromium.components.tab_group_sync.SavedTabGroup;
import org.chromium.components.tab_group_sync.TabGroupSyncService;
import org.chromium.components.tab_group_sync.TriggerSource;

import java.util.function.Supplier;

/**
 * Brave override of {@link TabGroupSyncRemoteObserver} that does not auto-open synced tab groups
 * while the "Enable tab groups" master switch is off.
 */
@NullMarked
public class BraveTabGroupSyncRemoteObserver extends TabGroupSyncRemoteObserver {
    public BraveTabGroupSyncRemoteObserver(
            TabModel tabModel,
            TabGroupSyncService tabGroupSyncService,
            LocalTabGroupMutationHelper localTabGroupMutationHelper,
            Callback<Boolean> enableLocalObserverCallback,
            PrefService prefService,
            Supplier<Boolean> isActiveWindowSupplier) {
        super(
                tabModel,
                tabGroupSyncService,
                localTabGroupMutationHelper,
                enableLocalObserverCallback,
                prefService,
                isActiveWindowSupplier);
    }

    @Override
    public void onTabGroupAdded(SavedTabGroup tabGroup, @TriggerSource int source) {
        // Do not auto-open any synced tab groups while Brave's "Enable tab groups" master switch is
        // off.
        if (!ChromeSharedPreferences.getInstance()
                .readBoolean(BravePreferenceKeys.BRAVE_TAB_GROUPS_FEATURE_ENABLED, true)) {
            return;
        }

        super.onTabGroupAdded(tabGroup, source);
    }
}
