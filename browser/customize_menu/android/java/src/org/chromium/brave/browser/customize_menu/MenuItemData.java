/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.customize_menu;

import android.graphics.drawable.Icon;
import android.os.Parcel;
import android.os.Parcelable;

import androidx.annotation.ColorRes;
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
    public final @Nullable Icon icon;
    public final @ColorRes int colorResId;
    public final boolean checked;

    public MenuItemData(
            @IdRes int id,
            @Nullable String title,
            @Nullable Icon icon,
            @ColorRes int colorResId,
            boolean checked) {
        this.id = id;
        this.title = title;
        this.icon = icon;
        this.colorResId = colorResId;
        this.checked = checked;
    }

    protected MenuItemData(Parcel in) {
        id = in.readInt();
        title = in.readString();
        icon = in.readParcelable(Icon.class.getClassLoader());
        colorResId = in.readInt();
        checked = in.readInt() == 1;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeInt(id);
        dest.writeString(title);
        dest.writeParcelable(icon, flags);
        dest.writeInt(colorResId);
        dest.writeInt(checked ? 1 : 0);
    }

    @Override
    public int describeContents() {
        return 0;
    }

    public static final Creator<MenuItemData> CREATOR =
            new Creator<MenuItemData>() {
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
