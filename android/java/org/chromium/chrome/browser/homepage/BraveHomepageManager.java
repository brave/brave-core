/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.homepage;

import org.chromium.chrome.browser.partnercustomizations.CloseBraveManager;

// see org.brave.bytecode.BraveHomepageManagerClassAdapter
public class BraveHomepageManager extends HomepageManager {
    @Override
    public boolean shouldCloseAppWithZeroTabs() {
        return CloseBraveManager.shouldCloseAppWithZeroTabs();
    }
}
