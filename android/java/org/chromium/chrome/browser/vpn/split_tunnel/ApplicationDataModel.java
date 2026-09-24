/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.vpn.split_tunnel;

import android.graphics.drawable.Drawable;

public class ApplicationDataModel {
    private final Drawable mIcon;
    private final String mName;
    private final String mPackageName;
    private final boolean mIsSystemApp;

    public ApplicationDataModel(
            Drawable icon, String name, String packageName, boolean isSystemApp) {
        this.mIcon = icon;
        this.mName = name;
        this.mPackageName = packageName;
        this.mIsSystemApp = isSystemApp;
    }

    public Drawable getIcon() {
        return mIcon;
    }

    public String getName() {
        return mName;
    }

    public String getPackageName() {
        return mPackageName;
    }

    public boolean isSystemApp() {
        return mIsSystemApp;
    }
}
