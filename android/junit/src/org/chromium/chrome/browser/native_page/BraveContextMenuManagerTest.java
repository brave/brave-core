/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.native_page;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.native_page.ContextMenuManager.ContextMenuItemId;
import org.chromium.chrome.browser.ntp.NtpUtil;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.ui.native_page.TouchEnabledDelegate;
import org.chromium.components.browser_ui.widget.ListItemBuilder;
import org.chromium.ui.listmenu.ListItemType;
import org.chromium.ui.listmenu.ListMenuItemProperties;
import org.chromium.ui.modelutil.MVCListAdapter;
import org.chromium.ui.modelutil.PropertyModel;

/**
 * Unit tests for {@link BraveContextMenuManager}. Verifies: (1) the "Open in new tab in group"
 * native page context menu item is only shown when the "Enable tab groups" master switch is on
 * (mirrors upstream {@link ContextMenuManager}'s test); (2) {@link
 * BraveContextMenuManager#customizeMenuModel} attaches icons to already-built standard items,
 * prepends "Add site", and appends the NTP top-sites widget-control items (behind a divider) when
 * the delegate is a {@link BraveNtpDelegate}; (3) clicks on Brave items are dispatched correctly.
 */
@RunWith(BaseRobolectricTestRunner.class)
public class BraveContextMenuManagerTest {
    @Rule public MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private NativePageNavigationDelegate mNavigationDelegate;
    @Mock private TouchEnabledDelegate mTouchEnabledDelegate;
    @Mock private ContextMenuManager.Delegate mDelegate;
    @Mock private BraveNtpDelegate mBraveDelegate;

    private BraveContextMenuManager mManager;

    @Before
    public void setUp() {
        mManager =
                new BraveContextMenuManager(
                        mNavigationDelegate, mTouchEnabledDelegate, () -> {}, "");
    }

    @After
    public void tearDown() {
        ChromeSharedPreferences.getInstance()
                .removeKey(BravePreferenceKeys.BRAVE_TAB_GROUPS_FEATURE_ENABLED);
    }

    @Test
    public void testOpenInNewTabInGroup_TabGroupsEnabled_Shown() {
        // Default state: the master switch is on, so the item is shown when upstream allows it.
        doReturn(true).when(mDelegate).isItemSupported(ContextMenuItemId.OPEN_IN_NEW_TAB_IN_GROUP);
        doReturn(true).when(mNavigationDelegate).isOpenInNewTabInGroupEnabled();

        assertTrue(mManager.shouldShowItem(ContextMenuItemId.OPEN_IN_NEW_TAB_IN_GROUP, mDelegate));
    }

    @Test
    public void testOpenInNewTabInGroup_TabGroupsDisabled_Hidden() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_TAB_GROUPS_FEATURE_ENABLED, false);
        doReturn(true).when(mDelegate).isItemSupported(ContextMenuItemId.OPEN_IN_NEW_TAB_IN_GROUP);
        doReturn(true).when(mNavigationDelegate).isOpenInNewTabInGroupEnabled();

        assertFalse(mManager.shouldShowItem(ContextMenuItemId.OPEN_IN_NEW_TAB_IN_GROUP, mDelegate));
    }

    @Test
    public void testOtherItemUnaffectedWhenTabGroupsDisabled() {
        // A non-group item stays visible regardless of the master switch.
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_TAB_GROUPS_FEATURE_ENABLED, false);
        doReturn(true).when(mDelegate).isItemSupported(ContextMenuItemId.OPEN_IN_NEW_TAB);

        assertTrue(mManager.shouldShowItem(ContextMenuItemId.OPEN_IN_NEW_TAB, mDelegate));
    }

    @Test
    public void testCustomizeMenuModel_addsIconToStandardItem() {
        MVCListAdapter.ModelList menuModel = new MVCListAdapter.ModelList();
        menuModel.add(standardItem(ContextMenuItemId.REMOVE));

        mManager.customizeMenuModel(menuModel, mDelegate);

        assertEquals(
                R.drawable.ic_trash,
                menuModel.get(0).model.get(ListMenuItemProperties.START_ICON_ID));
    }

    @Test
    public void testCustomizeMenuModel_unmappedItem_iconStaysZero() {
        MVCListAdapter.ModelList menuModel = new MVCListAdapter.ModelList();
        menuModel.add(standardItem(ContextMenuItemId.MOVE_UP));

        mManager.customizeMenuModel(menuModel, mDelegate);

        assertEquals(0, menuModel.get(0).model.get(ListMenuItemProperties.START_ICON_ID));
    }

    @Test
    public void testCustomizeMenuModel_skipsNonMenuItemEntries() {
        // A divider's PropertyModel doesn't carry MENU_ITEM_ID/START_ICON_ID; customizeMenuModel
        // must not touch it (would throw if it tried to read/write those keys).
        MVCListAdapter.ModelList menuModel = new MVCListAdapter.ModelList();
        menuModel.add(new MVCListAdapter.ListItem(ListItemType.DIVIDER, new PropertyModel()));

        mManager.customizeMenuModel(menuModel, mDelegate);

        assertEquals(1, menuModel.size());
    }

    @Test
    public void testCustomizeMenuModel_nonBraveDelegate_noExtraItems() {
        MVCListAdapter.ModelList menuModel = new MVCListAdapter.ModelList();
        menuModel.add(standardItem(ContextMenuItemId.REMOVE));

        mManager.customizeMenuModel(menuModel, mDelegate);

        assertEquals(1, menuModel.size());
    }

    @Test
    public void testCustomizeMenuModel_addSiteNotPrependedWhenNotSupported() {
        doReturn(false).when(mBraveDelegate).isBraveItemSupported(BraveNtpDelegate.BRAVE_ADD_SITE);
        MVCListAdapter.ModelList menuModel = new MVCListAdapter.ModelList();
        menuModel.add(standardItem(ContextMenuItemId.REMOVE));

        mManager.customizeMenuModel(menuModel, mBraveDelegate);

        assertEquals(1, menuModel.size());
    }

    @Test
    public void testCustomizeMenuModel_addSitePrependedWhenSupported() {
        doReturn(true).when(mBraveDelegate).isBraveItemSupported(BraveNtpDelegate.BRAVE_ADD_SITE);
        MVCListAdapter.ModelList menuModel = new MVCListAdapter.ModelList();
        menuModel.add(standardItem(ContextMenuItemId.REMOVE));

        mManager.customizeMenuModel(menuModel, mBraveDelegate);

        assertEquals(2, menuModel.size());
        assertEquals(
                BraveNtpDelegate.BRAVE_ADD_SITE,
                menuModel.get(0).model.get(ListMenuItemProperties.MENU_ITEM_ID));
        assertEquals(
                R.drawable.ic_browser_add,
                menuModel.get(0).model.get(ListMenuItemProperties.START_ICON_ID));
        assertEquals(
                ContextMenuItemId.REMOVE,
                menuModel.get(1).model.get(ListMenuItemProperties.MENU_ITEM_ID));
    }

    @Test
    public void testCustomizeMenuModel_noneSupported_noWidgetItemsAppended() {
        doReturn(false)
                .when(mBraveDelegate)
                .isBraveItemSupported(BraveNtpDelegate.BRAVE_SHOW_FREQUENT);
        doReturn(false)
                .when(mBraveDelegate)
                .isBraveItemSupported(BraveNtpDelegate.BRAVE_SHOW_SHORTCUTS);
        doReturn(false)
                .when(mBraveDelegate)
                .isBraveItemSupported(BraveNtpDelegate.BRAVE_HIDE_WIDGET);
        MVCListAdapter.ModelList menuModel = new MVCListAdapter.ModelList();
        menuModel.add(standardItem(ContextMenuItemId.REMOVE));

        mManager.customizeMenuModel(menuModel, mBraveDelegate);

        assertEquals(1, menuModel.size());
    }

    @Test
    public void testCustomizeMenuModel_allSupported_appendsDividerThenThreeItems() {
        doReturn(true)
                .when(mBraveDelegate)
                .isBraveItemSupported(BraveNtpDelegate.BRAVE_SHOW_FREQUENT);
        doReturn(true)
                .when(mBraveDelegate)
                .isBraveItemSupported(BraveNtpDelegate.BRAVE_SHOW_SHORTCUTS);
        doReturn(true)
                .when(mBraveDelegate)
                .isBraveItemSupported(BraveNtpDelegate.BRAVE_HIDE_WIDGET);
        doReturn(NtpUtil.TOP_SITES_MODE_FREQUENT)
                .when(mBraveDelegate)
                .getBraveTopSitesDisplayMode();
        doReturn(0).when(mBraveDelegate).getSelectedModeEndIconRes();
        MVCListAdapter.ModelList menuModel = new MVCListAdapter.ModelList();
        menuModel.add(standardItem(ContextMenuItemId.REMOVE));

        mManager.customizeMenuModel(menuModel, mBraveDelegate);

        // [0]=REMOVE (standard), [1]=divider, [2]=frequent, [3]=shortcuts, [4]=hide.
        assertEquals(5, menuModel.size());
        assertEquals(ListItemType.DIVIDER, menuModel.get(1).type);
        assertEquals(
                BraveNtpDelegate.BRAVE_SHOW_FREQUENT,
                menuModel.get(2).model.get(ListMenuItemProperties.MENU_ITEM_ID));
        assertEquals(
                BraveNtpDelegate.BRAVE_SHOW_SHORTCUTS,
                menuModel.get(3).model.get(ListMenuItemProperties.MENU_ITEM_ID));
        assertEquals(
                BraveNtpDelegate.BRAVE_HIDE_WIDGET,
                menuModel.get(4).model.get(ListMenuItemProperties.MENU_ITEM_ID));
    }

    @Test
    public void testCustomizeMenuModel_selectedModeGetsEndIcon() {
        doReturn(true)
                .when(mBraveDelegate)
                .isBraveItemSupported(BraveNtpDelegate.BRAVE_SHOW_FREQUENT);
        doReturn(true)
                .when(mBraveDelegate)
                .isBraveItemSupported(BraveNtpDelegate.BRAVE_SHOW_SHORTCUTS);
        doReturn(true)
                .when(mBraveDelegate)
                .isBraveItemSupported(BraveNtpDelegate.BRAVE_HIDE_WIDGET);
        // Current mode is SHORTCUTS (0).
        doReturn(NtpUtil.TOP_SITES_MODE_SHORTCUTS)
                .when(mBraveDelegate)
                .getBraveTopSitesDisplayMode();
        doReturn(R.drawable.ic_check_circle_filled)
                .when(mBraveDelegate)
                .getSelectedModeEndIconRes();
        MVCListAdapter.ModelList menuModel = new MVCListAdapter.ModelList();

        mManager.customizeMenuModel(menuModel, mBraveDelegate);

        // [0]=divider, [1]=frequent (not selected), [2]=shortcuts (selected), [3]=hide.
        assertEquals(0, menuModel.get(1).model.get(ListMenuItemProperties.END_ICON_ID));
        assertEquals(
                R.drawable.ic_check_circle_filled,
                menuModel.get(2).model.get(ListMenuItemProperties.END_ICON_ID));
        assertEquals(0, menuModel.get(3).model.get(ListMenuItemProperties.END_ICON_ID));
    }

    @Test
    public void testCustomizeMenuModel_partiallySupported_onlyIncludesSupportedItems() {
        doReturn(true)
                .when(mBraveDelegate)
                .isBraveItemSupported(BraveNtpDelegate.BRAVE_SHOW_FREQUENT);
        doReturn(false)
                .when(mBraveDelegate)
                .isBraveItemSupported(BraveNtpDelegate.BRAVE_SHOW_SHORTCUTS);
        doReturn(false)
                .when(mBraveDelegate)
                .isBraveItemSupported(BraveNtpDelegate.BRAVE_HIDE_WIDGET);
        doReturn(NtpUtil.TOP_SITES_MODE_FREQUENT)
                .when(mBraveDelegate)
                .getBraveTopSitesDisplayMode();
        doReturn(0).when(mBraveDelegate).getSelectedModeEndIconRes();
        MVCListAdapter.ModelList menuModel = new MVCListAdapter.ModelList();

        mManager.customizeMenuModel(menuModel, mBraveDelegate);

        // divider + the single supported item.
        assertEquals(2, menuModel.size());
        assertEquals(ListItemType.DIVIDER, menuModel.get(0).type);
        assertEquals(
                BraveNtpDelegate.BRAVE_SHOW_FREQUENT,
                menuModel.get(1).model.get(ListMenuItemProperties.MENU_ITEM_ID));
    }

    @Test
    public void testHandleMenuItemClick_addSite_dispatchesAndConsumes() {
        boolean handled =
                mManager.handleMenuItemClick(BraveNtpDelegate.BRAVE_ADD_SITE, mBraveDelegate);

        assertTrue(handled);
        verify(mBraveDelegate).braveAddSite();
    }

    @Test
    public void testHandleMenuItemClick_showFrequent_dispatchesAndConsumes() {
        boolean handled =
                mManager.handleMenuItemClick(BraveNtpDelegate.BRAVE_SHOW_FREQUENT, mBraveDelegate);

        assertTrue(handled);
        verify(mBraveDelegate).braveShowFrequent();
    }

    @Test
    public void testHandleMenuItemClick_showShortcuts_dispatchesAndConsumes() {
        boolean handled =
                mManager.handleMenuItemClick(BraveNtpDelegate.BRAVE_SHOW_SHORTCUTS, mBraveDelegate);

        assertTrue(handled);
        verify(mBraveDelegate).braveShowShortcuts();
    }

    @Test
    public void testHandleMenuItemClick_hideWidget_dispatchesAndConsumes() {
        boolean handled =
                mManager.handleMenuItemClick(BraveNtpDelegate.BRAVE_HIDE_WIDGET, mBraveDelegate);

        assertTrue(handled);
        verify(mBraveDelegate).braveHideWidget();
    }

    @Test
    public void testHandleMenuItemClick_standardItem_fallsThroughToSuper() {
        boolean handled = mManager.handleMenuItemClick(ContextMenuItemId.REMOVE, mBraveDelegate);

        assertTrue(handled);
        verify(mBraveDelegate).removeItem();
        verify(mBraveDelegate, never()).braveAddSite();
    }

    @Test
    public void testHandleMenuItemClick_nonBraveDelegate_standardItemStillWorks() {
        boolean handled = mManager.handleMenuItemClick(ContextMenuItemId.REMOVE, mDelegate);

        assertTrue(handled);
        verify(mDelegate).removeItem();
    }

    private static MVCListAdapter.ListItem standardItem(@ContextMenuItemId int itemId) {
        return new ListItemBuilder().withTitleRes(R.string.remove).withMenuId(itemId).build();
    }
}
