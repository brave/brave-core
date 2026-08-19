/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.view.View;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;

/**
 * Implemented by Settings fragments whose content does not use the standard preference list.
 *
 * <p>Returns the view that should receive the bottom system-bar inset.
 */
@NullMarked
public interface BottomInsetViewProvider {
    @Nullable View getBottomInsetView(View fragmentView);
}
