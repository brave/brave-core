/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ui.brave_tricks.checkbox_to_switch;

import android.content.Context;
import android.util.AttributeSet;

import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;

public class CheckBoxPreference extends ChromeSwitchPreference {
    public CheckBoxPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
    }
}
