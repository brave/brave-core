/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tab_group_sync;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.lenient;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoInteractions;
import static org.mockito.Mockito.when;

import org.junit.After;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import org.robolectric.annotation.Config;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.Token;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.preferences.Pref;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.TabClosureParams;
import org.chromium.chrome.browser.tabmodel.TabModel;
import org.chromium.chrome.browser.tabmodel.TabRemover;
import org.chromium.components.prefs.PrefService;
import org.chromium.components.tab_group_sync.LocalTabGroupId;
import org.chromium.components.tab_group_sync.SavedTabGroup;
import org.chromium.components.tab_group_sync.TabGroupSyncService;
import org.chromium.components.tab_group_sync.TabGroupUiActionHandler;

import java.util.LinkedHashSet;
import java.util.List;
import java.util.Set;

/** Unit tests for {@link BraveSyncedTabGroupHelper}. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class BraveSyncedTabGroupHelperUnitTest {
    private static final String SYNC_ID_1 = "sync-id-1";
    private static final String SYNC_ID_2 = "sync-id-2";
    private static final Token TAB_GROUP_ID_1 = new Token(1L, 2L);
    private static final Token TAB_GROUP_ID_2 = new Token(3L, 4L);

    @Rule public final MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private TabModel mTabModel;
    @Mock private TabRemover mTabRemover;
    @Mock private TabGroupSyncService mTabGroupSyncService;
    @Mock private TabGroupUiActionHandler mTabGroupUiActionHandler;
    @Mock private PrefService mPrefService;
    @Mock private Tab mSyncedGroupTab;
    @Mock private Tab mLocalOnlyGroupTab;

    @After
    public void tearDown() {
        ChromeSharedPreferences.getInstance()
                .removeKey(BravePreferenceKeys.BRAVE_TAB_GROUPS_FEATURE_ENABLED);
    }

    @Test
    public void testSyncedGroupsStayVisibleWhileTabGroupsAreEnabled() {
        // "Show synced tab groups" is off by default, and must not hide anything on its own.
        when(mPrefService.getBoolean(Pref.AUTO_OPEN_SYNCED_TAB_GROUPS)).thenReturn(false);

        assertTrue(BraveSyncedTabGroupHelper.areSyncedTabGroupsVisible(mPrefService));
    }

    @Test
    public void testSyncedGroupsAreHiddenWhenBothSettingsAreOff() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_TAB_GROUPS_FEATURE_ENABLED, false);
        when(mPrefService.getBoolean(Pref.AUTO_OPEN_SYNCED_TAB_GROUPS)).thenReturn(false);

        assertFalse(BraveSyncedTabGroupHelper.areSyncedTabGroupsVisible(mPrefService));
    }

    @Test
    public void testSyncedGroupsStayVisibleWithTabGroupsOffButShowSyncedOn() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_TAB_GROUPS_FEATURE_ENABLED, false);
        when(mPrefService.getBoolean(Pref.AUTO_OPEN_SYNCED_TAB_GROUPS)).thenReturn(true);

        assertTrue(BraveSyncedTabGroupHelper.areSyncedTabGroupsVisible(mPrefService));
    }

    @Test
    public void testHidesTheSyncedGroupsAndKeepsTheLocalOnes() {
        when(mTabModel.getAllTabGroupIds()).thenReturn(tabGroupIds(TAB_GROUP_ID_1, TAB_GROUP_ID_2));
        // Both groups hold a tab, so that being in sync is the only thing that tells them apart:
        // an empty group is skipped for having nothing to close, which would hide the check.
        when(mTabModel.getTabsInGroup(TAB_GROUP_ID_1)).thenReturn(List.of(mSyncedGroupTab));
        when(mTabModel.getTabsInGroup(TAB_GROUP_ID_2)).thenReturn(List.of(mLocalOnlyGroupTab));
        when(mTabModel.getTabRemover()).thenReturn(mTabRemover);
        when(mTabGroupSyncService.getGroup(new LocalTabGroupId(TAB_GROUP_ID_1)))
                .thenReturn(savedTabGroup(SYNC_ID_1));

        BraveSyncedTabGroupHelper.hideSyncedTabGroups(mTabModel, mTabGroupSyncService);

        // The group sync doesn't know about is left open, the synced one is hidden rather than
        // deleted so that it can be opened again.
        ArgumentCaptor<TabClosureParams> paramsCaptor =
                ArgumentCaptor.forClass(TabClosureParams.class);
        verify(mTabRemover).closeTabs(paramsCaptor.capture(), eq(false));
        TabClosureParams params = paramsCaptor.getValue();
        assertEquals(List.of(mSyncedGroupTab), params.tabs);
        assertTrue(params.hideTabGroups);
        assertTrue(params.isTabGroup);
    }

    @Test
    public void testHidesNothingWithoutSyncedGroups() {
        when(mTabModel.getAllTabGroupIds()).thenReturn(tabGroupIds(TAB_GROUP_ID_1));
        // The group has a tab, so nothing but the missing sync entry can keep it open. The remover
        // is only there to keep a regression from failing on a null one instead of on the check.
        when(mTabModel.getTabsInGroup(TAB_GROUP_ID_1)).thenReturn(List.of(mLocalOnlyGroupTab));
        lenient().when(mTabModel.getTabRemover()).thenReturn(mTabRemover);

        BraveSyncedTabGroupHelper.hideSyncedTabGroups(mTabModel, mTabGroupSyncService);

        verifyNoInteractions(mTabRemover);
    }

    @Test
    public void testOpensOnlyTheClosedGroups() {
        SavedTabGroup openGroup = savedTabGroup(SYNC_ID_2);
        openGroup.localId = new LocalTabGroupId(TAB_GROUP_ID_2);
        when(mTabGroupSyncService.getAllGroupIds()).thenReturn(new String[] {SYNC_ID_1, SYNC_ID_2});
        when(mTabGroupSyncService.getGroup(SYNC_ID_1)).thenReturn(savedTabGroup(SYNC_ID_1));
        when(mTabGroupSyncService.getGroup(SYNC_ID_2)).thenReturn(openGroup);

        BraveSyncedTabGroupHelper.showSyncedTabGroups(
                mTabGroupSyncService, mTabGroupUiActionHandler);

        verify(mTabGroupUiActionHandler).openTabGroup(SYNC_ID_1);
        verifyNoInteractions(mTabModel);
    }

    @Test
    public void testDoesNotOpenArchivedGroups() {
        SavedTabGroup archivedGroup = savedTabGroup(SYNC_ID_1);
        archivedGroup.archivalTimeMs = 1L;
        when(mTabGroupSyncService.getAllGroupIds()).thenReturn(new String[] {SYNC_ID_1});
        when(mTabGroupSyncService.getGroup(SYNC_ID_1)).thenReturn(archivedGroup);

        BraveSyncedTabGroupHelper.showSyncedTabGroups(
                mTabGroupSyncService, mTabGroupUiActionHandler);

        verifyNoInteractions(mTabGroupUiActionHandler);
    }

    private static SavedTabGroup savedTabGroup(String syncId) {
        SavedTabGroup savedTabGroup = new SavedTabGroup();
        savedTabGroup.syncId = syncId;
        return savedTabGroup;
    }

    private static Set<Token> tabGroupIds(Token... tabGroupIds) {
        return new LinkedHashSet<>(List.of(tabGroupIds));
    }
}
