/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
package org.chromium.brave.browser.custom_app_icons;

public enum CustomAppIconsEnum {
    ICON_DEFAULT(
            R.mipmap.ic_launcher_3d_round,
            R.string.launcher_default_title,
            "com.google.android.apps.chrome.Main"),
    ICON_3D(R.mipmap.ic_launcher_3d_round, R.string.launcher_3d_title, ".Icon3dAlias"),
    ICON_80S(R.mipmap.ic_launcher_80s_round, R.string.launcher_80s_title, ".Icon80sAlias"),
    ICON_AQUA(R.mipmap.ic_launcher_aqua_round, R.string.launcher_aqua_title, ".IconAquaAlias"),
    ICON_BAT(R.mipmap.ic_launcher_bat_round, R.string.launcher_bat_title, ".IconBatAlias"),
    ICON_HOLO(R.mipmap.ic_launcher_holo_round, R.string.launcher_holo_title, ".IconHoloAlias"),
    ICON_NEON(R.mipmap.ic_launcher_neon_round, R.string.launcher_neon_title, ".IconNeonAlias"),
    ICON_NETSCAPE(
            R.mipmap.ic_launcher_netscape_round,
            R.string.launcher_netscape_title,
            ".IconNetscapeAlias"),
    ICON_POPART(
            R.mipmap.ic_launcher_popart_round, R.string.launcher_popart_title, ".IconPopartAlias"),
    ICON_POPARTDARK(
            R.mipmap.ic_launcher_popartdark_round,
            R.string.launcher_popartdark_title,
            ".IconPopartDarkAlias"),
    ICON_SUPERNOVA(
            R.mipmap.ic_launcher_supernova_round,
            R.string.launcher_supernova_title,
            ".IconSupernovaAlias"),
    ICON_TERMINAL(
            R.mipmap.ic_launcher_terminal_round,
            R.string.launcher_terminal_title,
            ".IconTerminalAlias"),
    ICON_WINDOWS(
            R.mipmap.ic_launcher_windows_round,
            R.string.launcher_windows_title,
            ".IconWindowsAlias");

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
