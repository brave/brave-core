/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.language.settings;

import android.os.Bundle;

import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;

public class BraveLanguageSettings extends LanguageSettings {
    static final String TRANSLATE_SWITCH_KEY = "translate_switch";

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        super.onCreatePreferences(savedInstanceState, rootKey);
        ChromeSwitchPreference translateSwitch =
                (ChromeSwitchPreference) findPreference(TRANSLATE_SWITCH_KEY);
        if (translateSwitch != null) {
            getPreferenceScreen().removePreference(translateSwitch);
        }
    }
}
