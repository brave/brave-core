/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.customize_menu;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import android.os.Parcel;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.annotation.Config;

import org.chromium.base.test.BaseRobolectricTestRunner;

/** Unit tests for {@link MenuItemData}. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class MenuItemDataTest {
    @Test
    public void testConstructor() {
        MenuItemData data = new MenuItemData(R.id.new_tab_menu_id, "New Tab", true);

        assertEquals(R.id.new_tab_menu_id, data.id);
        assertEquals("New Tab", data.title);
        assertTrue(data.checked);
    }

    @Test
    public void testConstructorWithNullTitle() {
        MenuItemData data = new MenuItemData(R.id.downloads_menu_id, null, false);

        assertEquals(R.id.downloads_menu_id, data.id);
        assertNull(data.title);
        assertFalse(data.checked);
    }

    @Test
    public void testParcelable() {
        // Create original data.
        MenuItemData original = new MenuItemData(R.id.all_bookmarks_menu_id, "Bookmarks", true);

        // Write to parcel.
        Parcel parcel = Parcel.obtain();
        original.writeToParcel(parcel, 0);

        // Reset parcel for reading.
        parcel.setDataPosition(0);

        // Read from parcel.
        MenuItemData restored = MenuItemData.CREATOR.createFromParcel(parcel);

        // Verify data is preserved.
        assertEquals(original.id, restored.id);
        assertEquals(original.title, restored.title);
        assertEquals(original.checked, restored.checked);

        parcel.recycle();
    }

    @Test
    public void testParcelableWithNullTitle() {
        // Create original data with null title.
        MenuItemData original = new MenuItemData(R.id.brave_wallet_id, null, false);

        // Write to parcel.
        Parcel parcel = Parcel.obtain();
        original.writeToParcel(parcel, 0);

        // Reset parcel for reading.
        parcel.setDataPosition(0);

        // Read from parcel.
        MenuItemData restored = MenuItemData.CREATOR.createFromParcel(parcel);

        // Verify data is preserved.
        assertEquals(original.id, restored.id);
        assertNull(restored.title);
        assertEquals(original.checked, restored.checked);

        parcel.recycle();
    }

    @Test
    public void testMultipleParcelOperations() {
        // Test with multiple items to ensure parcel operations work correctly.
        MenuItemData[] originalData =
                new MenuItemData[] {
                    new MenuItemData(R.id.new_tab_menu_id, "New Tab", true),
                    new MenuItemData(R.id.downloads_menu_id, "Downloads", false),
                    new MenuItemData(R.id.brave_leo_id, null, true),
                    new MenuItemData(R.id.share_menu_id, "Share", false)
                };

        Parcel parcel = Parcel.obtain();

        // Write all items.
        for (MenuItemData item : originalData) {
            item.writeToParcel(parcel, 0);
        }

        // Reset and read all items.
        parcel.setDataPosition(0);
        MenuItemData[] restoredData = new MenuItemData[originalData.length];
        for (int i = 0; i < restoredData.length; i++) {
            restoredData[i] = MenuItemData.CREATOR.createFromParcel(parcel);
        }

        // Verify all items.
        for (int i = 0; i < originalData.length; i++) {
            assertEquals(originalData[i].id, restoredData[i].id);
            assertEquals(originalData[i].title, restoredData[i].title);
            assertEquals(originalData[i].checked, restoredData[i].checked);
        }

        parcel.recycle();
    }
}
