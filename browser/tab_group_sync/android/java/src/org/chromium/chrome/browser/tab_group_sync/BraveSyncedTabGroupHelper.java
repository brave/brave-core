/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tab_group_sync;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.ObserverList;
import org.chromium.base.Token;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.preferences.Pref;
import org.chromium.chrome.browser.tabmodel.TabClosureParams;
import org.chromium.chrome.browser.tabmodel.TabModel;
import org.chromium.components.prefs.PrefService;
import org.chromium.components.tab_group_sync.LocalTabGroupId;
import org.chromium.components.tab_group_sync.SavedTabGroup;
import org.chromium.components.tab_group_sync.TabGroupSyncService;
import org.chromium.components.tab_group_sync.TabGroupUiActionHandler;

import java.util.ArrayList;
import java.util.List;

/**
 * Shows and hides the synced tab groups, following the "Enable tab groups" and "Show synced tab
 * groups" settings. A group counts as synced once it has an entry in {@link TabGroupSyncService},
 * no matter which device created it. Groups that are not in sync are left as they are: they only
 * live in this tab model, so hiding them would lose them.
 */
@NullMarked
public class BraveSyncedTabGroupHelper {
    private static final ObserverList<Runnable> sSettingsObservers = new ObserverList<>();

    private BraveSyncedTabGroupHelper() {}

    /**
     * Registers {@code observer} to be run when the "Enable tab groups" switch changes. That switch
     * lives in shared preferences, which cannot be observed through {@link
     * org.chromium.base.shared_preferences.SharedPreferencesManager}, so the settings UI reports
     * the change through here instead.
     */
    public static void addSettingsObserver(Runnable observer) {
        sSettingsObservers.addObserver(observer);
    }

    /** Stops running {@code observer}, which must have been added by the call above. */
    public static void removeSettingsObserver(Runnable observer) {
        sSettingsObservers.removeObserver(observer);
    }

    /** Reports that the "Enable tab groups" switch changed. Called by the settings UI. */
    public static void notifySettingsChanged() {
        for (Runnable observer : sSettingsObservers) {
            observer.run();
        }
    }

    /**
     * Returns whether the synced tab groups belong in the tab model. While tab groups are enabled
     * they always do, and "Show synced tab groups" only decides whether a group arriving from
     * another device opens by itself. Once tab groups are disabled that setting is the only thing
     * keeping the synced groups around.
     */
    public static boolean areSyncedTabGroupsVisible(PrefService prefService) {
        return ChromeSharedPreferences.getInstance()
                        .readBoolean(BravePreferenceKeys.BRAVE_TAB_GROUPS_FEATURE_ENABLED, true)
                || prefService.getBoolean(Pref.AUTO_OPEN_SYNCED_TAB_GROUPS);
    }

    /** Opens every synced tab group that is not open yet. */
    public static void showSyncedTabGroups(
            TabGroupSyncService tabGroupSyncService,
            TabGroupUiActionHandler tabGroupUiActionHandler) {
        for (String syncId : tabGroupSyncService.getAllGroupIds()) {
            @Nullable SavedTabGroup savedTabGroup = tabGroupSyncService.getGroup(syncId);
            if (savedTabGroup == null) continue;
            // Skip the groups that are already open in some window, and the archived ones, which
            // are only ever restored on demand from the tab groups pane.
            if (savedTabGroup.localId != null || savedTabGroup.archivalTimeMs != null) continue;

            tabGroupUiActionHandler.openTabGroup(syncId);
        }
    }

    /** Closes every open synced tab group, keeping it in sync. */
    public static void hideSyncedTabGroups(
            TabModel tabModel, TabGroupSyncService tabGroupSyncService) {
        // Closing a group mutates the model, so iterate over a copy of the group IDs.
        List<Token> tabGroupIds = new ArrayList<>(tabModel.getAllTabGroupIds());
        for (Token tabGroupId : tabGroupIds) {
            // A group that sync doesn't know about cannot be brought back, and closing it would
            // delete it outright, so leave it open.
            if (tabGroupSyncService.getGroup(new LocalTabGroupId(tabGroupId)) == null) continue;

            var closeTabsBuilder = TabClosureParams.forCloseTabGroup(tabModel, tabGroupId);
            if (closeTabsBuilder == null) continue;

            tabModel.getTabRemover()
                    .closeTabs(
                            // Hiding instead of deleting keeps the group in sync, so that turning
                            // the setting back on can open it again.
                            closeTabsBuilder.hideTabGroups(true).allowUndo(false).build(),
                            /* allowDialog= */ false);
        }
    }
}
