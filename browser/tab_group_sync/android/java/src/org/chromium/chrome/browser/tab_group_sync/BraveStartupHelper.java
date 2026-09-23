/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tab_group_sync;

import org.chromium.build.annotations.NullMarked;

@NullMarked
public class BraveStartupHelper {

    // Replace StartupHelper.handleUnsavedLocalTabGroups with empty implementation.
    // The reason for this - otherwise groups created with `Open tabs in current group` option
    // are wiped after app restart.
    public void handleUnsavedLocalTabGroups() {}
}
