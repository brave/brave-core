/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.download.home.search;

import android.content.Context;

import org.chromium.base.Callback;
import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.download.internal.R;
import org.chromium.chrome.browser.util.BraveDynamicColors;

@NullMarked
public class BraveSearchBarCoordinator extends SearchBarCoordinator {
    public BraveSearchBarCoordinator(
            Context context, Callback<String> queryCallback, boolean autoFocusSearchBox) {
        super(context, queryCallback, autoFocusSearchBox);

        // When dynamic colors are disabled, use a background without color_primary to avoid
        // orange tint on light theme.
        if (!BraveDynamicColors.isDynamicColorsEnabled()) {
            getView().setBackgroundResource(R.drawable.no_dynamic_download_search_bar_background);
        }
    }
}
