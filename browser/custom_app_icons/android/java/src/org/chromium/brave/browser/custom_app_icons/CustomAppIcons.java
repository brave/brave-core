/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.custom_app_icons;

import android.util.SparseArray;

import androidx.annotation.DrawableRes;
import androidx.annotation.IntDef;
import androidx.annotation.StringRes;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * Manages custom app icons for Brave browser. This class provides functionality to: - Define
 * available icon types through an IntDef annotation - Store icon data (drawable resource, title,
 * activity alias) for each icon type - Access icon resources, titles and activity aliases - Get a
 * list of all available icons
 */
public class CustomAppIcons {

    public static final int ICON_DEFAULT = 0;
    public static final int ICON_3D = 1;
    public static final int ICON_80S = 2;
    public static final int ICON_AQUA = 3;
    public static final int ICON_BAT = 4;
    public static final int ICON_HOLO = 5;
    public static final int ICON_NEON = 6;
    public static final int ICON_NETSCAPE = 7;
    public static final int ICON_POPART = 8;
    public static final int ICON_POPARTDARK = 9;
    public static final int ICON_SUPERNOVA = 10;
    public static final int ICON_TERMINAL = 11;
    public static final int ICON_WINDOWS = 12;

    @IntDef({
        ICON_DEFAULT,
        ICON_3D,
        ICON_80S,
        ICON_AQUA,
        ICON_BAT,
        ICON_HOLO,
        ICON_NEON,
        ICON_NETSCAPE,
        ICON_POPART,
        ICON_POPARTDARK,
        ICON_SUPERNOVA,
        ICON_TERMINAL,
        ICON_WINDOWS
    })
    @Retention(RetentionPolicy.SOURCE)
    public @interface AppIconType {}

    public static class IconData {
        public final @DrawableRes int iconResId;
        public final @StringRes int titleResId;
        public final String activityAlias;

        public IconData(
                @DrawableRes int iconResId, @StringRes int titleResId, String activityAlias) {
            this.iconResId = iconResId;
            this.titleResId = titleResId;
            this.activityAlias = activityAlias;
        }
    }

    private static final SparseArray<IconData> sIconDataMap = new SparseArray<>();

    static {
        sIconDataMap.put(
                ICON_DEFAULT,
                new IconData(
                        R.mipmap.ic_launcher_3d_round,
                        R.string.launcher_default_title,
                        "com.google.android.apps.chrome.Main"));

        sIconDataMap.put(
                ICON_3D,
                new IconData(
                        R.mipmap.ic_launcher_3d_round, R.string.launcher_3d_title, ".Icon3dAlias"));
        sIconDataMap.put(
                ICON_80S,
                new IconData(
                        R.mipmap.ic_launcher_80s_round,
                        R.string.launcher_80s_title,
                        ".Icon80sAlias"));
        sIconDataMap.put(
                ICON_AQUA,
                new IconData(
                        R.mipmap.ic_launcher_aqua_round,
                        R.string.launcher_aqua_title,
                        ".IconAquaAlias"));
        sIconDataMap.put(
                ICON_BAT,
                new IconData(
                        R.mipmap.ic_launcher_bat_round,
                        R.string.launcher_bat_title,
                        ".IconBatAlias"));
        sIconDataMap.put(
                ICON_HOLO,
                new IconData(
                        R.mipmap.ic_launcher_holo_round,
                        R.string.launcher_holo_title,
                        ".IconHoloAlias"));
        sIconDataMap.put(
                ICON_NEON,
                new IconData(
                        R.mipmap.ic_launcher_neon_round,
                        R.string.launcher_neon_title,
                        ".IconNeonAlias"));
        sIconDataMap.put(
                ICON_NETSCAPE,
                new IconData(
                        R.mipmap.ic_launcher_netscape_round,
                        R.string.launcher_netscape_title,
                        ".IconNetscapeAlias"));
        sIconDataMap.put(
                ICON_POPART,
                new IconData(
                        R.mipmap.ic_launcher_popart_round,
                        R.string.launcher_popart_title,
                        ".IconPopartAlias"));
        sIconDataMap.put(
                ICON_POPARTDARK,
                new IconData(
                        R.mipmap.ic_launcher_popartdark_round,
                        R.string.launcher_popartdark_title,
                        ".IconPopartDarkAlias"));
        sIconDataMap.put(
                ICON_SUPERNOVA,
                new IconData(
                        R.mipmap.ic_launcher_supernova_round,
                        R.string.launcher_supernova_title,
                        ".IconSupernovaAlias"));
        sIconDataMap.put(
                ICON_TERMINAL,
                new IconData(
                        R.mipmap.ic_launcher_terminal_round,
                        R.string.launcher_terminal_title,
                        ".IconTerminalAlias"));
        sIconDataMap.put(
                ICON_WINDOWS,
                new IconData(
                        R.mipmap.ic_launcher_windows_round,
                        R.string.launcher_windows_title,
                        ".IconWindowsAlias"));
    }

    /**
     * Gets the drawable resource ID for the icon of the specified type.
     *
     * @param type The app icon type to get the drawable for
     * @return The drawable resource ID for the icon
     */
    public static @DrawableRes int getIcon(@AppIconType int type) {
        return sIconDataMap.get(type).iconResId;
    }

    /**
     * Gets the string resource ID for the title of the specified icon type.
     *
     * @param type The app icon type to get the title for
     * @return The string resource ID for the title
     */
    public static @StringRes int getTitle(@AppIconType int type) {
        return sIconDataMap.get(type).titleResId;
    }

    /**
     * Gets the activity alias string for the specified icon type. This is used to enable/disable
     * different app icons in the launcher.
     *
     * @param type The app icon type to get the alias for
     * @return The activity alias string
     */
    public static String getAlias(@AppIconType int type) {
        return sIconDataMap.get(type).activityAlias;
    }

    /**
     * Gets an array of all available icon types.
     *
     * @return An array of all available icon types
     */
    public static @AppIconType int[] getAllIcons() {
        int size = sIconDataMap.size();
        @AppIconType int[] icons = new int[size];

        for (int i = 0; i < size; i++) {
            icons[i] = sIconDataMap.keyAt(i);
        }

        return icons;
    }
}
