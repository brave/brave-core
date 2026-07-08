/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tab_group_sync;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import org.robolectric.annotation.Config;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.preferences.Pref;
import org.chromium.chrome.browser.tabmodel.TabModel;
import org.chromium.components.prefs.PrefService;
import org.chromium.components.tab_group_sync.OpeningSource;
import org.chromium.components.tab_group_sync.SavedTabGroup;
import org.chromium.components.tab_group_sync.TabGroupSyncService;
import org.chromium.components.tab_group_sync.TriggerSource;

import java.util.function.Supplier;

/** Unit tests for {@link BraveTabGroupSyncRemoteObserver}. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class BraveTabGroupSyncRemoteObserverUnitTest {
    @Rule public MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private TabModel mTabModel;
    @Mock private TabGroupSyncService mTabGroupSyncService;
    @Mock private LocalTabGroupMutationHelper mLocalMutationHelper;
    @Mock private PrefService mPrefService;
    @Mock private Supplier<Boolean> mIsActiveWindowSupplier;

    private BraveTabGroupSyncRemoteObserver mObserver;

    @Before
    public void setUp() {
        mObserver =
                new BraveTabGroupSyncRemoteObserver(
                        mTabModel,
                        mTabGroupSyncService,
                        mLocalMutationHelper,
                        enable -> {},
                        mPrefService,
                        mIsActiveWindowSupplier);
    }

    @After
    public void tearDown() {
        ChromeSharedPreferences.getInstance()
                .removeKey(BravePreferenceKeys.BRAVE_TAB_GROUPS_FEATURE_ENABLED);
    }

    private static SavedTabGroup newSyncedTabGroup() {
        SavedTabGroup tabGroup = new SavedTabGroup();
        tabGroup.syncId = "sync-id";
        return tabGroup;
    }

    private void setTabGroupsEnabled(boolean enabled) {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_TAB_GROUPS_FEATURE_ENABLED, enabled);
    }

    @Test
    public void testAutoOpensWhenTabGroupsEnabled() {
        setTabGroupsEnabled(true);
        when(mPrefService.getBoolean(Pref.AUTO_OPEN_SYNCED_TAB_GROUPS)).thenReturn(true);
        when(mIsActiveWindowSupplier.get()).thenReturn(true);

        mObserver.onTabGroupAdded(newSyncedTabGroup(), TriggerSource.REMOTE);

        verify(mLocalMutationHelper)
                .createNewTabGroup(any(), eq(OpeningSource.AUTO_OPENED_FROM_SYNC));
    }

    @Test
    public void testDoesNotAutoOpenWhenTabGroupsDisabled() {
        setTabGroupsEnabled(false);

        mObserver.onTabGroupAdded(newSyncedTabGroup(), TriggerSource.REMOTE);

        verify(mLocalMutationHelper, never()).createNewTabGroup(any(), anyInt());
    }

    @Test
    public void testDoesNotAutoOpenCollaborationGroupWhenTabGroupsDisabled() {
        setTabGroupsEnabled(false);
        SavedTabGroup tabGroup = newSyncedTabGroup();
        tabGroup.collaborationId = "collaboration-id";

        mObserver.onTabGroupAdded(tabGroup, TriggerSource.REMOTE);

        verify(mLocalMutationHelper, never()).createNewTabGroup(any(), anyInt());
    }
}
