/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.top;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.when;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.supplier.ObservableSuppliers;
import org.chromium.base.supplier.SettableMonotonicObservableSupplier;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.base.test.util.Features.DisableFeatures;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.incognito.IncognitoUtils;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.TabModel;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.toolbar.R;
import org.chromium.ui.listmenu.ListItemType;
import org.chromium.ui.listmenu.ListMenuItemProperties;
import org.chromium.ui.modelutil.MVCListAdapter.ListItem;
import org.chromium.ui.modelutil.MVCListAdapter.ModelList;

/**
 * Unit tests for {@link BraveTabSwitcherActionMenuCoordinator}. Verifies the tab group entries are
 * only present when the "Enable tab groups" master switch is on. Mirrors upstream {@link
 * TabSwitcherActionMenuCoordinator}'s test.
 */
@RunWith(BaseRobolectricTestRunner.class)
@DisableFeatures(ChromeFeatureList.ANDROID_OPEN_INCOGNITO_AS_WINDOW)
public class BraveTabSwitcherActionMenuCoordinatorTest {
    @Rule public final MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private Profile mProfile;
    @Mock private TabModelSelector mTabModelSelector;
    @Mock private TabModel mIncognitoTabModel;
    @Mock private TabModel mNormalTabModel;
    @Mock private Tab mTab;

    private final SettableMonotonicObservableSupplier<TabModelSelector> mTabModelSelectorSupplier =
            ObservableSuppliers.createMonotonic();
    private final SettableMonotonicObservableSupplier<Tab> mCurrentTabSupplier =
            ObservableSuppliers.createMonotonic();
    private BraveTabSwitcherActionMenuCoordinator mCoordinator;

    @Before
    public void setUp() {
        mTabModelSelectorSupplier.set(mTabModelSelector);
        mCurrentTabSupplier.set(mTab);

        IncognitoUtils.setEnabledForTesting(true);
        IncognitoUtils.setShouldOpenIncognitoAsWindowForTesting(true);

        when(mTabModelSelector.getModel(true)).thenReturn(mIncognitoTabModel);
        when(mTabModelSelector.getModel(false)).thenReturn(mNormalTabModel);
        when(mTabModelSelector.getCurrentModel()).thenReturn(mNormalTabModel);
        when(mTabModelSelector.getCurrentTabSupplier()).thenReturn(mCurrentTabSupplier);
        when(mTabModelSelector.isTabStateInitialized()).thenReturn(true);
        when(mTabModelSelector.isIncognitoBrandedModelSelected()).thenReturn(false);
        when(mIncognitoTabModel.getCount()).thenReturn(0);
        when(mNormalTabModel.isTabModelRestored()).thenReturn(true);

        mCoordinator =
                new BraveTabSwitcherActionMenuCoordinator(mProfile, mTabModelSelectorSupplier);
    }

    @After
    public void tearDown() {
        ChromeSharedPreferences.getInstance()
                .removeKey(BravePreferenceKeys.BRAVE_TAB_GROUPS_FEATURE_ENABLED);
    }

    @Test
    public void testBuildMenuItems_TabGroupsEnabled_ShowsAddToNewGroup() {
        // Default state: the master switch is on, so "Add to new group" is present.
        when(mNormalTabModel.getTabGroupCount()).thenReturn(0);

        ModelList items = mCoordinator.buildMenuItems();

        assertEquals(5, items.size());
        assertEquals(R.id.add_tab_to_new_group_menu_id, getMenuItemId(items, 4));
    }

    @Test
    public void testBuildMenuItems_TabGroupsDisabled_NoGroups_OmitsAddToNewGroup() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_TAB_GROUPS_FEATURE_ENABLED, false);
        when(mNormalTabModel.getTabGroupCount()).thenReturn(0);

        ModelList items = mCoordinator.buildMenuItems();

        // Close, Divider, New Tab, New Incognito - the "Add to new group" entry is stripped.
        assertEquals(4, items.size());
        assertFalse(hasMenuItem(items, R.id.add_tab_to_new_group_menu_id));
    }

    @Test
    public void testBuildMenuItems_TabGroupsDisabled_GroupsExist_OmitsAddToGroup() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_TAB_GROUPS_FEATURE_ENABLED, false);
        when(mNormalTabModel.getTabGroupCount()).thenReturn(1);

        ModelList items = mCoordinator.buildMenuItems();

        assertFalse(hasMenuItem(items, R.id.add_tab_to_group_menu_id));
    }

    @Test
    public void testBuildMenuItems_TabGroupsEnabled_GroupsExist_ShowsAddToGroup() {
        when(mNormalTabModel.getTabGroupCount()).thenReturn(1);

        ModelList items = mCoordinator.buildMenuItems();

        assertTrue(hasMenuItem(items, R.id.add_tab_to_group_menu_id));
    }

    private static int getMenuItemId(ModelList items, int index) {
        return items.get(index).model.get(ListMenuItemProperties.MENU_ITEM_ID);
    }

    private static boolean hasMenuItem(ModelList items, int menuItemId) {
        for (ListItem item : items) {
            if (item.type == ListItemType.MENU_ITEM
                    && item.model.get(ListMenuItemProperties.MENU_ITEM_ID) == menuItemId) {
                return true;
            }
        }
        return false;
    }
}
