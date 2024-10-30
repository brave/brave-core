/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.password_manager.settings;

import org.chromium.chrome.browser.settings.BravePreferenceFragment;

public abstract class BravePasswordSettingsBase extends BravePreferenceFragment {
    public void createCheckPasswords() {
        // Do nothing here as we don't have check passwords option in Brave.
    }
}
