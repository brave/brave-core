/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
package org.chromium.brave.browser.custom_app_icons;

public enum CustomAppIconsEnum {
    ICON_DEFAULT(
            R.drawable.ic_launcher_3d,
            R.string.launcher_default_title,
            "com.google.android.apps.chrome.Main"),
    ICON_3D(R.drawable.ic_launcher_3d, R.string.launcher_3d_title, ".Icon3dAlias"),
    ICON_80S(R.drawable.ic_launcher_80s, R.string.launcher_80s_title, ".Icon80sAlias"),
    ICON_AQUA(R.drawable.ic_launcher_aqua, R.string.launcher_aqua_title, ".IconAquaAlias"),
    ICON_BAT(R.drawable.ic_launcher_bat, R.string.launcher_bat_title, ".IconBatAlias"),
    ICON_HOLO(R.drawable.ic_launcher_holo, R.string.launcher_holo_title, ".IconHoloAlias"),
    ICON_NEON(R.drawable.ic_launcher_neon, R.string.launcher_neon_title, ".IconNeonAlias"),
    ICON_NETSCAPE(
            R.drawable.ic_launcher_netscape,
            R.string.launcher_netscape_title,
            ".IconNetscapeAlias"),
    ICON_POPART(R.drawable.ic_launcher_popart, R.string.launcher_popart_title, ".IconPopartAlias"),
    ICON_POPARTDARK(
            R.drawable.ic_launcher_popartdark,
            R.string.launcher_popartdark_title,
            ".IconPopartDarkAlias"),
    ICON_SUPERNOVA(
            R.drawable.ic_launcher_supernova,
            R.string.launcher_supernova_title,
            ".IconSupernovaAlias"),
    ICON_TERMINAL(
            R.drawable.ic_launcher_terminal,
            R.string.launcher_terminal_title,
            ".IconTerminalAlias"),
    ICON_WINDOWS(
            R.drawable.ic_launcher_windows, R.string.launcher_windows_title, ".IconWindowsAlias");

    private final int mIcon;
    private final int mDesc;
    private final String mAlias;

    CustomAppIconsEnum(int icon, int desc, String alias) {
        this.mIcon = icon;
        this.mDesc = desc;
        this.mAlias = alias;
    }

    public int getIcon() {
        return mIcon;
    }

    public int getDesc() {
        return mDesc;
    }

    public String getAlias() {
        return mAlias;
    }
}
