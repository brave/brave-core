/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.omnibox.styles;

import android.content.Context;

import androidx.annotation.Px;

import org.chromium.chrome.R;

public class BraveOmniboxResourceProvider {
    public static @Px int getToolbarSidePaddingForNtp(Context context) {
        return context.getResources().getDimensionPixelSize(R.dimen.brave_toolbar_edge_padding_ntp);
    }
}
