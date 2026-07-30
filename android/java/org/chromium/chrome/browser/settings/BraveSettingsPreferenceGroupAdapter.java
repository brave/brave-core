/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import androidx.preference.PreferenceGroupAdapter;
import androidx.preference.PreferenceScreen;
import androidx.preference.PreferenceViewHolder;

import org.chromium.build.annotations.NonNull;
import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.theme.BraveDynamicColors;
import org.chromium.components.browser_ui.settings.BraveSettingsIconTintUtils;

@NullMarked
public class BraveSettingsPreferenceGroupAdapter extends PreferenceGroupAdapter {
    public BraveSettingsPreferenceGroupAdapter(PreferenceScreen preferenceScreen) {
        super(preferenceScreen);
    }

    @Override
    public void onBindViewHolder(@NonNull PreferenceViewHolder holder, int position) {
        super.onBindViewHolder(holder, position);
        BraveSettingsIconTintUtils.applyIconTint(
                holder, BraveDynamicColors.isDynamicColorsEnabled());
    }
}
