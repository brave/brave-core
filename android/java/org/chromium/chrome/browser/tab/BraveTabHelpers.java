/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tab;

import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.BraveSwipeRefreshHandler;

// see org.brave.bytecode.BraveTabHelpersClassAdapter
@NullMarked
public class BraveTabHelpers {
    static void initWebContentsHelpers(Tab tab) {
        TabHelpers.initWebContentsHelpers(tab);
        BraveSwipeRefreshHandler.from(tab);
    }
}
