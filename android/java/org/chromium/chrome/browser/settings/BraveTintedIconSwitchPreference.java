/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.content.Context;
import android.util.AttributeSet;

import androidx.preference.PreferenceViewHolder;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.theme.BraveDynamicColors;
import org.chromium.components.browser_ui.settings.BraveSettingsIconTintUtils;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;

/** A Brave switch preference that tints its icon for enabled and disabled states. */
@NullMarked
public class BraveTintedIconSwitchPreference extends ChromeSwitchPreference {
    public BraveTintedIconSwitchPreference(Context context, @Nullable AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    public void onBindViewHolder(PreferenceViewHolder holder) {
        super.onBindViewHolder(holder);
        BraveSettingsIconTintUtils.applyIconTint(
                holder, BraveDynamicColors.isDynamicColorsEnabled());
    }
}
