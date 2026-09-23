/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tasks.tab_management;

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
import org.chromium.base.supplier.ObservableSuppliers;
import org.chromium.base.supplier.SettableNonNullObservableSupplier;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.base.test.util.Features.DisableFeatures;
import org.chromium.chrome.browser.bookmarks.BookmarkModel;
import org.chromium.chrome.browser.bookmarks.TabBookmarker;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.multiwindow.MultiInstanceOrchestrator;
import org.chromium.chrome.browser.multiwindow.MultiInstanceOrchestratorFactory;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.share.ShareDelegate;
import org.chromium.chrome.browser.share.send_tab_to_self.SendTabToSelfAndroidBridge;
import org.chromium.chrome.browser.share.send_tab_to_self.SendTabToSelfAndroidBridgeJni;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabId;
import org.chromium.chrome.browser.tabmodel.TabModel;
import org.chromium.chrome.browser.tabmodel.TabRemover;
import org.chromium.chrome.browser.tasks.tab_management.TabGridContextMenuCoordinator.ShowTabListEditor;
import org.chromium.chrome.tab_ui.R;
import org.chromium.components.collaboration.CollaborationService;
import org.chromium.components.tab_group_sync.TabGroupSyncService;
import org.chromium.ui.base.TestActivity;
import org.chromium.ui.listmenu.ListMenuItemProperties;
import org.chromium.ui.modelutil.MVCListAdapter.ModelList;
import org.chromium.url.GURL;

import java.util.function.Supplier;

/**
 * Unit tests for {@link BraveTabGridContextMenuCoordinator}. Verifies the tab group entry is only
 * present when the "Enable tab groups" master switch is on. Mirrors upstream {@link
 * TabGridContextMenuCoordinator}'s test.
 */
@RunWith(BaseRobolectricTestRunner.class)
@DisableFeatures(ChromeFeatureList.SEND_TAB_TO_SELF_EXTRA_ENTRY_POINTS)
public class BraveTabGridContextMenuCoordinatorTest {
    private static @TabId final int TAB_ID = 1;
    private static final String LOCALHOST_URL = "localhost://";

    @Rule public MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Rule
    public ActivityScenarioRule<TestActivity> mActivityScenarioRule =
            new ActivityScenarioRule<>(TestActivity.class);

    @Mock private TabBookmarker mTabBookmarker;
    @Mock private TabGroupListBottomSheetCoordinator mTabGroupListBottomSheetCoordinator;
    @Mock private TabGroupCreationDialogManager mTabGroupCreationDialogManager;
    @Mock private Supplier<ShareDelegate> mShareDelegateSupplier;
    @Mock private TabGroupSyncService mTabGroupSyncService;
    @Mock private CollaborationService mCollaborationService;
    @Mock private TabModel mTabModel;
    @Mock private TabRemover mTabRemover;
    @Mock private Tab mTab;
    @Mock private ShareDelegate mShareDelegate;
    @Mock private Profile mProfile;
    @Mock private BookmarkModel mBookmarkModel;
    @Mock private ShowTabListEditor mShowTabListEditor;
    @Mock private MultiInstanceOrchestrator mMultiInstanceOrchestrator;
    @Mock private SendTabToSelfAndroidBridge.Natives mSendTabToSelfAndroidBridgeNatives;

    private BraveTabGridContextMenuCoordinator mCoordinator;
    private ModelList mMenuItemList;
    private Activity mActivity;
    private Token mTabGroupId;
    private SettableNonNullObservableSupplier<TabBookmarker> mTabBookmarkerSupplier;

    @Before
    public void setUp() {
        mTabGroupId = Token.createRandom();
        mTabBookmarkerSupplier = ObservableSuppliers.createNonNull(mTabBookmarker);

        when(mTabModel.getTabGroupCount()).thenReturn(1);
        when(mTabModel.getTabRemover()).thenReturn(mTabRemover);
        when(mTabModel.getProfile()).thenReturn(mProfile);
        when(mShareDelegateSupplier.get()).thenReturn(mShareDelegate);
        when(mTab.getTabGroupId()).thenReturn(mTabGroupId);

        BookmarkModel.setInstanceForTesting(mBookmarkModel);
        MultiInstanceOrchestratorFactory.setInstanceForTesting(mMultiInstanceOrchestrator);

        mActivityScenarioRule.getScenario().onActivity(activity -> mActivity = activity);
        mActivity.setTheme(R.style.Theme_BrowserUI_DayNight);
        mCoordinator =
                new BraveTabGridContextMenuCoordinator(
                        mActivity,
                        mTabBookmarkerSupplier,
                        mProfile,
                        mTabModel,
                        mTabGroupListBottomSheetCoordinator,
                        mTabGroupCreationDialogManager,
                        mShareDelegateSupplier,
                        mTabGroupSyncService,
                        mCollaborationService,
                        mShowTabListEditor);
        mMenuItemList = new ModelList();
        when(mTabModel.getTabById(anyInt())).thenReturn(mTab);
        when(mTab.getId()).thenReturn(TAB_ID);
        when(mBookmarkModel.hasBookmarkIdForTab(any())).thenReturn(false);
        SendTabToSelfAndroidBridgeJni.setInstanceForTesting(mSendTabToSelfAndroidBridgeNatives);
        when(mSendTabToSelfAndroidBridgeNatives.getEntryPointDisplayReason(any(), any()))
                .thenReturn(null);
        when(mTab.getUrl()).thenReturn(new GURL(LOCALHOST_URL));
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

        assertEquals(7, mMenuItemList.size());
        assertEquals(R.string.menu_move_tab_to_group, getMenuItemTitleId(0));
        assertEquals(R.string.add_to_bookmarks, getMenuItemTitleId(1));
        assertEquals(R.string.share, getMenuItemTitleId(2));
        assertEquals(R.string.pin_tab, getMenuItemTitleId(3));
        assertEquals(R.string.mute_site, getMenuItemTitleId(4));
        assertEquals(R.string.select_tab, getMenuItemTitleId(5));
        assertEquals(R.string.close_tab, getMenuItemTitleId(6));
    }

    @Test
    public void testBuildMenuActionItems_tabGroupsDisabled_hidesGroupItem() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_TAB_GROUPS_FEATURE_ENABLED, false);

        mCoordinator.buildMenuActionItems(mMenuItemList, TAB_ID);

        // The group entry is stripped; the remaining items keep their order.
        assertEquals(6, mMenuItemList.size());
        assertEquals(R.string.add_to_bookmarks, getMenuItemTitleId(0));
        assertEquals(R.string.share, getMenuItemTitleId(1));
        assertEquals(R.string.pin_tab, getMenuItemTitleId(2));
        assertEquals(R.string.mute_site, getMenuItemTitleId(3));
        assertEquals(R.string.select_tab, getMenuItemTitleId(4));
        assertEquals(R.string.close_tab, getMenuItemTitleId(5));
    }

    private int getMenuItemTitleId(int menuItemListIndex) {
        return mMenuItemList.get(menuItemListIndex).model.get(ListMenuItemProperties.TITLE_ID);
    }
}
