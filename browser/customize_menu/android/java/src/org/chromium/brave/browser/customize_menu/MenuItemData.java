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
 * Parcelable data class representing a menu item for customization. This can be passed between
 * activities via Intent extras.
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
