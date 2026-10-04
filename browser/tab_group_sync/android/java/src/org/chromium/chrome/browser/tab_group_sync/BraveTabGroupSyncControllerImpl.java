/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tab_group_sync;

import org.chromium.base.CallbackController;
import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.preferences.Pref;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.tabmodel.TabModelUtils;
import org.chromium.components.prefs.PrefChangeRegistrar;
import org.chromium.components.prefs.PrefService;
import org.chromium.components.tab_group_sync.TabGroupSyncService;

import java.util.function.Supplier;

/**
 * Brave override of {@link TabGroupSyncControllerImpl} that keeps the synced tab groups in step
 * with the "Enable tab groups" and "Show synced tab groups" settings: the groups are hidden as soon
 * as they are not supposed to be visible, and opened again once they are. Upstream only consults
 * the "Show synced tab groups" setting when a new group arrives from sync, which leaves the groups
 * that are already open behind.
 */
@NullMarked
public class BraveTabGroupSyncControllerImpl extends TabGroupSyncControllerImpl {
    private final CallbackController mCallbackController = new CallbackController();
    private final TabModelSelector mTabModelSelector;
    private final TabGroupSyncService mTabGroupSyncService;
    private final PrefService mPrefService;
    private final Supplier<Boolean> mIsActiveWindowSupplier;
    private final PrefChangeRegistrar mPrefChangeRegistrar;

    private final Runnable mSettingsObserver = this::onSyncedTabGroupVisibilityChanged;

    private final TabGroupSyncService.Observer mSyncBackendInitObserver =
            new TabGroupSyncService.Observer() {
                @Override
                public void onInitialized() {
                    mTabGroupSyncService.removeObserver(mSyncBackendInitObserver);
                    // Catch up with a settings change that happened while this window wasn't
                    // running. Only hiding is applied: opening every synced group on each start
                    // would undo the groups the user closed by hand.
                    if (!areSyncedTabGroupsVisible()) hideSyncedTabGroups();
                }
            };

    public BraveTabGroupSyncControllerImpl(
            TabModelSelector tabModelSelector,
            TabGroupSyncService tabGroupSyncService,
            PrefService prefService,
            Supplier<Boolean> isActiveWindowSupplier) {
        super(tabModelSelector, tabGroupSyncService, prefService, isActiveWindowSupplier);
        mTabModelSelector = tabModelSelector;
        mTabGroupSyncService = tabGroupSyncService;
        mPrefService = prefService;
        mIsActiveWindowSupplier = isActiveWindowSupplier;

        mPrefChangeRegistrar = new PrefChangeRegistrar(prefService);
        mPrefChangeRegistrar.addObserver(
                Pref.AUTO_OPEN_SYNCED_TAB_GROUPS, this::onSyncedTabGroupVisibilityChanged);
        BraveSyncedTabGroupHelper.addSettingsObserver(mSettingsObserver);
        // The sync back end is what knows the groups, and the tab model has to be there to hold
        // them, so wait for both before looking at the current state.
        TabModelUtils.runOnTabStateInitialized(
                tabModelSelector,
                mCallbackController.makeCancelable(
                        selector -> mTabGroupSyncService.addObserver(mSyncBackendInitObserver)));
    }

    @Override
    public void destroy() {
        mCallbackController.destroy();
        mTabGroupSyncService.removeObserver(mSyncBackendInitObserver);
        BraveSyncedTabGroupHelper.removeSettingsObserver(mSettingsObserver);
        mPrefChangeRegistrar.destroy();
        super.destroy();
    }

    private void onSyncedTabGroupVisibilityChanged() {
        if (!areSyncedTabGroupsVisible()) {
            hideSyncedTabGroups();
            return;
        }

        // Every window watches the settings, but a group can only be opened in one of them.
        if (!mIsActiveWindowSupplier.get()) return;

        BraveSyncedTabGroupHelper.showSyncedTabGroups(mTabGroupSyncService, this);
    }

    private void hideSyncedTabGroups() {
        BraveSyncedTabGroupHelper.hideSyncedTabGroups(
                mTabModelSelector.getModel(/* incognito= */ false), mTabGroupSyncService);
    }

    private boolean areSyncedTabGroupsVisible() {
        return BraveSyncedTabGroupHelper.areSyncedTabGroupsVisible(mPrefService);
    }
}
