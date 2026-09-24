/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tasks.tab_management.pinned_tabs_strip;

import static org.junit.Assert.assertEquals;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.Mockito.when;

import android.app.Activity;

import androidx.test.ext.junit.rules.ActivityScenarioRule;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.Token;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.browser.bookmarks.BookmarkModel;
import org.chromium.chrome.browser.bookmarks.TabBookmarker;
import org.chromium.chrome.browser.multiwindow.MultiInstanceOrchestrator;
import org.chromium.chrome.browser.multiwindow.MultiInstanceOrchestratorFactory;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabId;
import org.chromium.chrome.browser.tabmodel.TabModel;
import org.chromium.chrome.browser.tabmodel.TabRemover;
import org.chromium.chrome.browser.tasks.tab_management.TabGroupCreationDialogManager;
import org.chromium.chrome.browser.tasks.tab_management.TabGroupListBottomSheetCoordinator;
import org.chromium.chrome.tab_ui.R;
import org.chromium.components.collaboration.CollaborationService;
import org.chromium.components.tab_group_sync.TabGroupSyncService;
import org.chromium.ui.base.TestActivity;
import org.chromium.ui.listmenu.ListMenuItemProperties;
import org.chromium.ui.modelutil.MVCListAdapter.ModelList;
import org.chromium.url.GURL;

import java.util.function.Supplier;

/**
 * Unit tests for {@link BravePinnedTabStripItemContextMenuCoordinator}. Verifies the tab group
 * entry is only present when the "Enable tab groups" master switch is on. Mirrors upstream {@link
 * PinnedTabStripItemContextMenuCoordinator}'s test.
 */
@RunWith(BaseRobolectricTestRunner.class)
public class BravePinnedTabStripItemContextMenuCoordinatorTest {
    private static @TabId final int TAB_ID = 1;
    private static final String LOCALHOST_URL = "localhost://";

    @Rule public MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Rule
    public ActivityScenarioRule<TestActivity> mActivityScenarioRule =
            new ActivityScenarioRule<>(TestActivity.class);

    @Mock private Supplier<TabBookmarker> mTabBookmarkerSupplier;
    @Mock private TabBookmarker mTabBookmarker;
    @Mock private TabGroupListBottomSheetCoordinator mTabGroupListBottomSheetCoordinator;
    @Mock private TabGroupCreationDialogManager mTabGroupCreationDialogManager;
    @Mock private TabGroupSyncService mTabGroupSyncService;
    @Mock private CollaborationService mCollaborationService;
    @Mock private TabModel mTabModel;
    @Mock private TabRemover mTabRemover;
    @Mock private Tab mTab;
    @Mock private Profile mProfile;
    @Mock private BookmarkModel mBookmarkModel;
    @Mock private MultiInstanceOrchestrator mMultiInstanceOrchestrator;

    private BravePinnedTabStripItemContextMenuCoordinator mCoordinator;
    private ModelList mMenuItemList;
    private Activity mActivity;
    private GURL mUrl;
    private Token mTabGroupId;

    @Before
    public void setUp() {
        mTabGroupId = Token.createRandom();
        when(mTabBookmarkerSupplier.get()).thenReturn(mTabBookmarker);

        when(mTabModel.getTabGroupCount()).thenReturn(1);
        when(mTabModel.getTabRemover()).thenReturn(mTabRemover);
        when(mTabModel.getProfile()).thenReturn(mProfile);
        when(mTab.getTabGroupId()).thenReturn(mTabGroupId);

        BookmarkModel.setInstanceForTesting(mBookmarkModel);
        MultiInstanceOrchestratorFactory.setInstanceForTesting(mMultiInstanceOrchestrator);

        mActivityScenarioRule.getScenario().onActivity(activity -> mActivity = activity);
        mActivity.setTheme(R.style.Theme_BrowserUI_DayNight);
        mCoordinator =
                new BravePinnedTabStripItemContextMenuCoordinator(
                        mActivity,
                        mProfile,
                        mTabBookmarkerSupplier,
                        mTabModel,
                        mTabGroupListBottomSheetCoordinator,
                        mTabGroupCreationDialogManager,
                        mTabGroupSyncService,
                        mCollaborationService);
        mMenuItemList = new ModelList();
        when(mTabModel.getTabById(anyInt())).thenReturn(mTab);
        when(mTab.getId()).thenReturn(TAB_ID);
        when(mBookmarkModel.hasBookmarkIdForTab(any())).thenReturn(false);
        mUrl = new GURL(LOCALHOST_URL);
        when(mTab.getUrl()).thenReturn(mUrl);
    }

    @After
    public void tearDown() {
        ChromeSharedPreferences.getInstance()
                .removeKey(BravePreferenceKeys.BRAVE_TAB_GROUPS_FEATURE_ENABLED);
    }

    @Test
    public void testBuildMenuActionItems_tabGroupsEnabled_showsGroupItem() {
        // Default state: the master switch is on, so the "move to group" entry is present.
        mCoordinator.buildMenuActionItems(mMenuItemList, TAB_ID);

        assertEquals(4, mMenuItemList.size());
        assertEquals(R.string.menu_move_tab_to_group, getMenuItemTitleId(0));
        assertEquals(R.string.add_to_bookmarks, getMenuItemTitleId(1));
        assertEquals(R.string.unpin_tab, getMenuItemTitleId(2));
        assertEquals(R.string.close_tab, getMenuItemTitleId(3));
    }

    @Test
    public void testBuildMenuActionItems_tabGroupsDisabled_hidesGroupItem() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_TAB_GROUPS_FEATURE_ENABLED, false);

        mCoordinator.buildMenuActionItems(mMenuItemList, TAB_ID);

        // The group entry is stripped; the remaining items keep their order.
        assertEquals(3, mMenuItemList.size());
        assertEquals(R.string.add_to_bookmarks, getMenuItemTitleId(0));
        assertEquals(R.string.unpin_tab, getMenuItemTitleId(1));
        assertEquals(R.string.close_tab, getMenuItemTitleId(2));
    }

    private int getMenuItemTitleId(int menuItemListIndex) {
        return mMenuItemList.get(menuItemListIndex).model.get(ListMenuItemProperties.TITLE_ID);
    }
}
