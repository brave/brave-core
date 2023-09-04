/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.components.browser_ui.settings.brave_tricks.checkbox_to_switch;

import android.content.Context;
import android.util.AttributeSet;

import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;

/**
 * Redefinition of CheckBoxPreference to act as ChromeSwitchPreference in fact.
 */
public class ChromeBaseCheckBoxPreference extends ChromeSwitchPreference {
    public ChromeBaseCheckBoxPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
    }
}
