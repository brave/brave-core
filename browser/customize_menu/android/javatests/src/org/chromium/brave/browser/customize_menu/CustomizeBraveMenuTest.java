/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.customize_menu;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.res.Resources;
import android.graphics.drawable.Drawable;
import android.os.Bundle;

import androidx.test.core.app.ApplicationProvider;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.ExpectedException;
import org.junit.runner.RunWith;
import org.mockito.MockitoAnnotations;
import org.robolectric.annotation.Config;

import org.chromium.base.BraveFeatureList;
import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.base.test.util.Features.DisableFeatures;
import org.chromium.build.BuildConfig;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.ui.appmenu.AppMenuHandler;
import org.chromium.chrome.browser.ui.appmenu.AppMenuItemProperties;
import org.chromium.ui.modelutil.MVCListAdapter;
import org.chromium.ui.modelutil.PropertyModel;

import java.util.ArrayList;

@DisableFeatures({
    BraveFeatureList.BRAVE_SHRED,
})

/** Unit tests for {@link CustomizeBraveMenu}. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class CustomizeBraveMenuTest {

    private Context mContext;
    private MVCListAdapter.ModelList mModelList;

    @Rule public ExpectedException thrown = ExpectedException.none();
    private Resources mResources;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mContext = ApplicationProvider.getApplicationContext();
        mModelList = new MVCListAdapter.ModelList();
        mResources = ApplicationProvider.getApplicationContext().getResources();
    }

    @Test
    public void testApplyCustomization_RemovesHiddenItems() {
        // Add test items to model list.
        PropertyModel item1 =
                new PropertyModel.Builder(AppMenuItemProperties.ALL_KEYS)
                        .with(AppMenuItemProperties.MENU_ITEM_ID, R.id.new_tab_menu_id)
                        .with(AppMenuItemProperties.TITLE, "New Tab")
                        .build();
        PropertyModel item2 =
                new PropertyModel.Builder(AppMenuItemProperties.ALL_KEYS)
                        .with(AppMenuItemProperties.MENU_ITEM_ID, R.id.downloads_menu_id)
                        .with(AppMenuItemProperties.TITLE, "Downloads")
                        .build();

        mModelList.add(new MVCListAdapter.ListItem(AppMenuHandler.AppMenuItemType.STANDARD, item1));
        mModelList.add(new MVCListAdapter.ListItem(AppMenuHandler.AppMenuItemType.STANDARD, item2));

        // Hide download item.
        String key =
                String.format(
                        BravePreferenceKeys.CUSTOMIZABLE_BRAVE_MENU_ITEM_ID_FORMAT,
                        mResources.getResourceEntryName(R.id.downloads_menu_id));
        ChromeSharedPreferences.getInstance().writeBoolean(key, false);

        // Apply customization.
        CustomizeBraveMenu.applyCustomization(mResources, mModelList);

        // Verify only one visible item remains.
        assertEquals(1, mModelList.size());
        assertEquals(
                R.id.new_tab_menu_id,
                mModelList.get(0).model.get(AppMenuItemProperties.MENU_ITEM_ID));
    }

    @Test
    public void testApplyCustomization_PreservesCustomizeAndSettingsItems() {
        // Add customize menu and settings items (should not be removed).
        PropertyModel customizeItem =
                new PropertyModel.Builder(AppMenuItemProperties.ALL_KEYS)
                        .with(
                                AppMenuItemProperties.MENU_ITEM_ID,
                                CustomizeBraveMenu.BRAVE_CUSTOMIZE_ITEM_ID)
                        .with(AppMenuItemProperties.TITLE, "Customize")
                        .build();
        PropertyModel settingsItem =
                new PropertyModel.Builder(AppMenuItemProperties.ALL_KEYS)
                        .with(AppMenuItemProperties.MENU_ITEM_ID, R.id.preferences_id)
                        .with(AppMenuItemProperties.TITLE, "Settings")
                        .build();

        mModelList.add(
                new MVCListAdapter.ListItem(
                        AppMenuHandler.AppMenuItemType.STANDARD, customizeItem));
        mModelList.add(
                new MVCListAdapter.ListItem(AppMenuHandler.AppMenuItemType.STANDARD, settingsItem));

        // Apply customization.
        CustomizeBraveMenu.applyCustomization(mResources, mModelList);

        // Both items should remain.
        assertEquals(2, mModelList.size());
    }

    @Test
    public void testSanitizeSeparators_RemovesLeadingSeparators() {
        // Add leading separators.
        mModelList.add(
                new MVCListAdapter.ListItem(
                        AppMenuHandler.AppMenuItemType.DIVIDER, new PropertyModel()));
        mModelList.add(
                new MVCListAdapter.ListItem(
                        AppMenuHandler.AppMenuItemType.DIVIDER, new PropertyModel()));

        PropertyModel item =
                new PropertyModel.Builder(AppMenuItemProperties.ALL_KEYS)
                        .with(AppMenuItemProperties.MENU_ITEM_ID, R.id.new_tab_menu_id)
                        .with(AppMenuItemProperties.TITLE, "New Tab")
                        .build();
        mModelList.add(new MVCListAdapter.ListItem(AppMenuHandler.AppMenuItemType.STANDARD, item));

        // Sanitize.
        CustomizeBraveMenu.sanitizeSeparators(mModelList);

        // Should only have the menu item, no leading separators.
        assertEquals(1, mModelList.size());
        assertEquals(AppMenuHandler.AppMenuItemType.STANDARD, mModelList.get(0).type);
    }

    @Test
    public void testSanitizeSeparators_RemovesTrailingSeparators() {
        PropertyModel item =
                new PropertyModel.Builder(AppMenuItemProperties.ALL_KEYS)
                        .with(AppMenuItemProperties.MENU_ITEM_ID, R.id.new_tab_menu_id)
                        .with(AppMenuItemProperties.TITLE, "New Tab")
                        .build();
        mModelList.add(new MVCListAdapter.ListItem(AppMenuHandler.AppMenuItemType.STANDARD, item));

        // Add trailing separators.
        mModelList.add(
                new MVCListAdapter.ListItem(
                        AppMenuHandler.AppMenuItemType.DIVIDER, new PropertyModel()));
        mModelList.add(
                new MVCListAdapter.ListItem(
                        AppMenuHandler.AppMenuItemType.DIVIDER, new PropertyModel()));

        // Sanitize.
        CustomizeBraveMenu.sanitizeSeparators(mModelList);

        // Should only have the menu item, no trailing separators.
        assertEquals(1, mModelList.size());
        assertEquals(AppMenuHandler.AppMenuItemType.STANDARD, mModelList.get(0).type);
    }

    @Test
    public void testSanitizeSeparators_CollapsesAdjacentSeparators() {
        PropertyModel item1 =
                new PropertyModel.Builder(AppMenuItemProperties.ALL_KEYS)
                        .with(AppMenuItemProperties.MENU_ITEM_ID, R.id.new_tab_menu_id)
                        .with(AppMenuItemProperties.TITLE, "New Tab")
                        .build();
        PropertyModel item2 =
                new PropertyModel.Builder(AppMenuItemProperties.ALL_KEYS)
                        .with(AppMenuItemProperties.MENU_ITEM_ID, R.id.downloads_menu_id)
                        .with(AppMenuItemProperties.TITLE, "Downloads")
                        .build();

        mModelList.add(new MVCListAdapter.ListItem(AppMenuHandler.AppMenuItemType.STANDARD, item1));

        // Add multiple separators.
        mModelList.add(
                new MVCListAdapter.ListItem(
                        AppMenuHandler.AppMenuItemType.DIVIDER, new PropertyModel()));
        mModelList.add(
                new MVCListAdapter.ListItem(
                        AppMenuHandler.AppMenuItemType.DIVIDER, new PropertyModel()));
        mModelList.add(
                new MVCListAdapter.ListItem(
                        AppMenuHandler.AppMenuItemType.DIVIDER, new PropertyModel()));

        mModelList.add(new MVCListAdapter.ListItem(AppMenuHandler.AppMenuItemType.STANDARD, item2));

        // Sanitize.
        CustomizeBraveMenu.sanitizeSeparators(mModelList);

        // Should have: item1, single separator, item2.
        assertEquals(3, mModelList.size());
        assertEquals(AppMenuHandler.AppMenuItemType.STANDARD, mModelList.get(0).type);
        assertEquals(AppMenuHandler.AppMenuItemType.DIVIDER, mModelList.get(1).type);
        assertEquals(AppMenuHandler.AppMenuItemType.STANDARD, mModelList.get(2).type);
    }

    @Test
    public void testSanitizeSeparators_EmptyList() {
        // Test with empty list - should not crash.
        CustomizeBraveMenu.sanitizeSeparators(mModelList);
        assertEquals(0, mModelList.size());
    }

    @Test
    public void testSanitizeSeparators_OnlySeparators() {
        // Add only separators.
        mModelList.add(
                new MVCListAdapter.ListItem(
                        AppMenuHandler.AppMenuItemType.DIVIDER, new PropertyModel()));
        mModelList.add(
                new MVCListAdapter.ListItem(
                        AppMenuHandler.AppMenuItemType.DIVIDER, new PropertyModel()));

        // Sanitize.
        CustomizeBraveMenu.sanitizeSeparators(mModelList);

        // Should remove all separators.
        assertEquals(0, mModelList.size());
    }

    @Test
    public void testPopulateBundle() {
        Bundle bundle = new Bundle();
        MVCListAdapter.ModelList menuItems = new MVCListAdapter.ModelList();
        MVCListAdapter.ModelList pageActions = new MVCListAdapter.ModelList();

        // Add test items.
        PropertyModel menuItem =
                new PropertyModel.Builder(AppMenuItemProperties.ALL_KEYS)
                        .with(AppMenuItemProperties.MENU_ITEM_ID, R.id.new_tab_menu_id)
                        .with(AppMenuItemProperties.TITLE, "New Tab")
                        .build();
        menuItems.add(
                new MVCListAdapter.ListItem(AppMenuHandler.AppMenuItemType.STANDARD, menuItem));

        PropertyModel pageAction =
                new PropertyModel.Builder(AppMenuItemProperties.ALL_KEYS)
                        .with(AppMenuItemProperties.MENU_ITEM_ID, R.id.share_menu_id)
                        .with(AppMenuItemProperties.TITLE, "Share")
                        .build();
        pageActions.add(
                new MVCListAdapter.ListItem(AppMenuHandler.AppMenuItemType.STANDARD, pageAction));

        // Populate bundle.
        Bundle result =
                CustomizeBraveMenu.populateBundle(mResources, bundle, menuItems, pageActions);

        // Verify bundle contains the lists.
        assertNotNull(result);
        // Should return same bundle instance.
        assertEquals(bundle, result);

        ArrayList<MenuItemData> mainList =
                bundle.getParcelableArrayList(CustomizeBraveMenu.KEY_MAIN_MENU_ITEM_LIST);
        ArrayList<MenuItemData> pageList =
                bundle.getParcelableArrayList(CustomizeBraveMenu.KEY_PAGE_ACTION_ITEM_LIST);

        assertNotNull(mainList);
        assertNotNull(pageList);
        assertEquals(1, mainList.size());
        assertEquals(1, pageList.size());

        // Verify menu item data.
        MenuItemData mainData = mainList.get(0);
        assertEquals(R.id.new_tab_menu_id, mainData.id);
        assertEquals("New Tab", mainData.title);
        assertTrue(mainData.checked);

        MenuItemData pageData = pageList.get(0);
        assertEquals(R.id.share_menu_id, pageData.id);
        assertEquals("Share", pageData.title);
        assertTrue(pageData.checked);
    }

    @Test
    public void testIsVisible_DefaultTrue() {
        assertTrue(CustomizeBraveMenu.isVisible(mResources, R.id.new_tab_menu_id));
        assertTrue(CustomizeBraveMenu.isVisible(mResources, R.id.new_incognito_tab_menu_id));
        assertTrue(CustomizeBraveMenu.isVisible(mResources, R.id.downloads_menu_id));
        assertTrue(CustomizeBraveMenu.isVisible(mResources, R.id.all_bookmarks_menu_id));
        assertTrue(CustomizeBraveMenu.isVisible(mResources, R.id.open_history_menu_id));
        assertTrue(CustomizeBraveMenu.isVisible(mResources, R.id.brave_wallet_id));
        assertTrue(CustomizeBraveMenu.isVisible(mResources, R.id.brave_leo_id));
    }

    @Test
    public void testIsVisible_ExplicitlyHidden() {
        // Download menu d preference set to false.
        String key =
                String.format(
                        BravePreferenceKeys.CUSTOMIZABLE_BRAVE_MENU_ITEM_ID_FORMAT,
                        mResources.getResourceEntryName(R.id.downloads_menu_id));
        ChromeSharedPreferences.getInstance().writeBoolean(key, false);

        assertFalse(CustomizeBraveMenu.isVisible(mResources, R.id.downloads_menu_id));
    }

    @Test
    public void testGetDrawableResFromMenuItemId_ValidIds() {
        // Test common menu items.
        assertEquals(
                R.drawable.ic_new_tab_page,
                CustomizeBraveMenu.getDrawableResFromMenuItemId(R.id.new_tab_menu_id));
        assertEquals(
                R.drawable.brave_menu_new_private_tab,
                CustomizeBraveMenu.getDrawableResFromMenuItemId(R.id.new_incognito_tab_menu_id));
        assertEquals(
                R.drawable.brave_menu_downloads,
                CustomizeBraveMenu.getDrawableResFromMenuItemId(R.id.downloads_menu_id));
        assertEquals(
                R.drawable.brave_menu_bookmarks,
                CustomizeBraveMenu.getDrawableResFromMenuItemId(R.id.all_bookmarks_menu_id));
        assertEquals(
                R.drawable.brave_menu_history,
                CustomizeBraveMenu.getDrawableResFromMenuItemId(R.id.open_history_menu_id));
        assertEquals(
                R.drawable.ic_crypto_wallets,
                CustomizeBraveMenu.getDrawableResFromMenuItemId(R.id.brave_wallet_id));
        assertEquals(
                R.drawable.ic_brave_ai,
                CustomizeBraveMenu.getDrawableResFromMenuItemId(R.id.brave_leo_id));
    }

    @SuppressLint("ResourceType")
    @Test
    public void testGetDrawableResFromMenuItemId_InvalidId() {
        // We assert on debug builds only.
        if (BuildConfig.ENABLE_ASSERTS) {
            thrown.expect(AssertionError.class);
        }
        // Test with invalid ID - should return 0.
        assertEquals(0, CustomizeBraveMenu.getDrawableResFromMenuItemId(999999));
    }

    @Test
    public void testGetStandardizedMenuIcon_InvalidDrawableRes() {
        // Test that it handles invalid resource ID gracefully.
        final Drawable[] result = new Drawable[1];
        CustomizeBraveMenu.getStandardizedMenuIconAsync(
                mContext, 0, 24, drawable -> result[0] = drawable);
        assertNull(result[0]);
    }

    @Test
    public void testPropagateMenuItemExtras_NullPreference() {
        Bundle bundle = new Bundle();
        bundle.putParcelableArrayList(
                CustomizeBraveMenu.KEY_MAIN_MENU_ITEM_LIST, new ArrayList<>());

        // Should not crash with null preference.
        CustomizeBraveMenu.propagateMenuItemExtras(null, bundle);
    }

    @Test
    public void testPropagateMenuItemExtras_NullBundle() {
        // Should not crash with null bundle.
        CustomizeBraveMenu.propagateMenuItemExtras(null, null);
    }

    @Test
    public void testApplyCustomization_WithSeparators() {
        // Create a complex menu with items and separators.
        PropertyModel item1 =
                new PropertyModel.Builder(AppMenuItemProperties.ALL_KEYS)
                        .with(AppMenuItemProperties.MENU_ITEM_ID, R.id.new_tab_menu_id)
                        .with(AppMenuItemProperties.TITLE, "New Tab")
                        .build();
        PropertyModel item2 =
                new PropertyModel.Builder(AppMenuItemProperties.ALL_KEYS)
                        .with(AppMenuItemProperties.MENU_ITEM_ID, R.id.downloads_menu_id)
                        .with(AppMenuItemProperties.TITLE, "Downloads")
                        .build();
        PropertyModel item3 =
                new PropertyModel.Builder(AppMenuItemProperties.ALL_KEYS)
                        .with(AppMenuItemProperties.MENU_ITEM_ID, R.id.all_bookmarks_menu_id)
                        .with(AppMenuItemProperties.TITLE, "Bookmarks")
                        .build();
        PropertyModel separator =
                new PropertyModel.Builder(AppMenuItemProperties.ALL_KEYS)
                        .with(AppMenuItemProperties.MENU_ITEM_ID, R.id.divider_line_id)
                        .build();

        // Build list with separators.
        mModelList.add(new MVCListAdapter.ListItem(AppMenuHandler.AppMenuItemType.STANDARD, item1));
        mModelList.add(
                new MVCListAdapter.ListItem(AppMenuHandler.AppMenuItemType.DIVIDER, separator));
        mModelList.add(new MVCListAdapter.ListItem(AppMenuHandler.AppMenuItemType.STANDARD, item2));
        mModelList.add(
                new MVCListAdapter.ListItem(AppMenuHandler.AppMenuItemType.DIVIDER, separator));
        mModelList.add(
                new MVCListAdapter.ListItem(AppMenuHandler.AppMenuItemType.DIVIDER, separator));
        mModelList.add(new MVCListAdapter.ListItem(AppMenuHandler.AppMenuItemType.STANDARD, item3));

        // Apply customization.
        CustomizeBraveMenu.applyCustomization(mResources, mModelList);

        // Should have: item1, separator, item2, separator, item3 (no adjacent separators).
        assertEquals(5, mModelList.size());
        assertEquals(AppMenuHandler.AppMenuItemType.STANDARD, mModelList.get(0).type);
        assertEquals(AppMenuHandler.AppMenuItemType.DIVIDER, mModelList.get(1).type);
        assertEquals(AppMenuHandler.AppMenuItemType.STANDARD, mModelList.get(2).type);
        assertEquals(AppMenuHandler.AppMenuItemType.DIVIDER, mModelList.get(3).type);
        assertEquals(AppMenuHandler.AppMenuItemType.STANDARD, mModelList.get(4).type);
    }
}
