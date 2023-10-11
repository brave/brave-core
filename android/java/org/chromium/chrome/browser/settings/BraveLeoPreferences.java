/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.os.Bundle;

import org.chromium.chrome.R;
import org.chromium.components.browser_ui.settings.SettingsUtils;

public class BraveLeoPreferences extends BravePreferenceFragment {

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        requireActivity().setTitle(R.string.menu_brave_leo);
        SettingsUtils.addPreferencesFromResource(this, R.xml.brave_leo_preferences);
    }
}
