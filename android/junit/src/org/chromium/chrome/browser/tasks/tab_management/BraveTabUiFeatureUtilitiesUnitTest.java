/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tasks.tab_management;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.After;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.ui.listmenu.ListItemType;
import org.chromium.ui.listmenu.ListMenuItemProperties;
import org.chromium.ui.modelutil.MVCListAdapter.ListItem;
import org.chromium.ui.modelutil.MVCListAdapter.ModelList;
import org.chromium.ui.modelutil.PropertyModel;

/** Unit tests for {@link BraveTabUiFeatureUtilities}. */
@RunWith(BaseRobolectricTestRunner.class)
public class BraveTabUiFeatureUtilitiesUnitTest {
    private static final int ID_A = 1001;
    private static final int ID_B = 1002;
    private static final int ID_C = 1003;

    @After
    public void tearDown() {
        ChromeSharedPreferences.getInstance()
                .removeKey(BravePreferenceKeys.BRAVE_TAB_GROUPS_FEATURE_ENABLED);
    }

    private static ListItem menuItem(int menuId) {
        return new ListItem(
                ListItemType.MENU_ITEM,
                new PropertyModel.Builder(ListMenuItemProperties.ALL_KEYS)
                        .with(ListMenuItemProperties.MENU_ITEM_ID, menuId)
                        .build());
    }

    private static ListItem divider() {
        return new ListItem(
                ListItemType.DIVIDER,
                new PropertyModel.Builder(ListMenuItemProperties.ALL_KEYS).build());
    }

    @Test
    public void testRemoveMenuItems_removesMatchingIdsAndKeepsOthers() {
        ModelList list = new ModelList();
        list.add(menuItem(ID_A));
        list.add(divider());
        list.add(menuItem(ID_B));
        list.add(menuItem(ID_C));

        BraveTabUiFeatureUtilities.removeMenuItems(list, ID_A, ID_C);

        assertEquals(2, list.size());
        assertEquals(ListItemType.DIVIDER, list.get(0).type);
        assertEquals(ID_B, list.get(1).model.get(ListMenuItemProperties.MENU_ITEM_ID));
    }

    @Test
    public void testRemoveMenuItems_noMatchIsNoOp() {
        ModelList list = new ModelList();
        list.add(menuItem(ID_A));
        list.add(menuItem(ID_B));

        BraveTabUiFeatureUtilities.removeMenuItems(list, ID_C);

        assertEquals(2, list.size());
    }

    @Test
    public void testIsTabGroupsEnabled_defaultsTrue_andReflectsPref() {
        assertTrue(BraveTabUiFeatureUtilities.isTabGroupsEnabled());

        BraveTabUiFeatureUtilities.setTabGroupsEnabled(false);
        assertFalse(BraveTabUiFeatureUtilities.isTabGroupsEnabled());

        BraveTabUiFeatureUtilities.setTabGroupsEnabled(true);
        assertTrue(BraveTabUiFeatureUtilities.isTabGroupsEnabled());
    }
}
