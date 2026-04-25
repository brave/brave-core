/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;

public class NavigationItem {
    @NonNull private final String mTitle;
    @NonNull private final Fragment mFragment;

    public NavigationItem(@NonNull final String title, @NonNull final Fragment fragment) {
        this.mTitle = title;
        this.mFragment = fragment;
    }

    @NonNull
    public String getTitle() {
        return mTitle;
    }

    @NonNull
    public Fragment getFragment() {
        return mFragment;
    }
}
