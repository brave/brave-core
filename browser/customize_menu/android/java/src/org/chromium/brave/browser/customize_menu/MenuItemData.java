/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.customize_menu;

import android.os.Parcel;
import android.os.Parcelable;

import androidx.annotation.IdRes;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;

/**
 * Parcelable data class representing a menu item for the Brave menu customization feature.
 *
 * <p>This class encapsulates the essential information about a menu item that needs to be
 * transferred between activities when navigating to the customize menu settings screen. It contains
 * the menu item's resource ID, display title, and current visibility state.
 *
 * <p>The class implements {@link Parcelable} to enable efficient serialization and transfer via
 * {@link android.content.Intent} extras when launching the {@link
 * org.chromium.brave.browser.customize_menu.settings.BraveCustomizeMenuPreferenceFragment}.
 *
 * <p><strong>Note on Icons:</strong> This class does not include drawable icons because {@link
 * android.graphics.drawable.Drawable} objects cannot be parceled across activity boundaries.
 * Instead, the settings screen uses the {@code id} field with {@link
 * org.chromium.brave.browser.customize_menu.CustomizeBraveMenu#getDrawableResFromMenuItemId(int)}
 * to retrieve the appropriate icon drawable resource.
 *
 * <p><strong>Usage Example:</strong>
 *
 * <pre>{@code
 * // Create menu item data for "New Tab" item that is currently visible
 * MenuItemData newTabItem = new MenuItemData(
 *     R.id.new_tab_menu_id,
 *     "New Tab",
 *     true
 * );
 *
 * // Add to bundle for transfer to settings screen
 * ArrayList<MenuItemData> menuItems = new ArrayList<>();
 * menuItems.add(newTabItem);
 * bundle.putParcelableArrayList(CustomizeBraveMenu.KEY_MAIN_MENU_ITEM_LIST, menuItems);
 * }</pre>
 */
@NullMarked
public class MenuItemData implements Parcelable {
    public final @IdRes int id;
    public final @Nullable String title;
    public final boolean checked;

    public MenuItemData(@IdRes int id, @Nullable String title, boolean checked) {
        this.id = id;
        this.title = title;
        this.checked = checked;
    }

    protected MenuItemData(Parcel in) {
        id = in.readInt();
        title = in.readString();
        checked = in.readInt() == 1;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeInt(id);
        dest.writeString(title);
        dest.writeInt(checked ? 1 : 0);
    }

    @Override
    public int describeContents() {
        return 0;
    }

    public static final Creator<MenuItemData> CREATOR =
            new Creator<>() {
                @Override
                public MenuItemData createFromParcel(Parcel in) {
                    return new MenuItemData(in);
                }

                @Override
                public MenuItemData[] newArray(int size) {
                    return new MenuItemData[size];
                }
            };
}
