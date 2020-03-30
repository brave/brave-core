/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.night_mode.settings;

import android.content.Context;
import android.support.v7.preference.PreferenceViewHolder;
import android.util.AttributeSet;
import android.view.View;

import org.chromium.chrome.R;
import org.chromium.base.BuildInfo;

public class BraveRadioButtonGroupThemePreference extends RadioButtonGroupThemePreference {
    public BraveRadioButtonGroupThemePreference(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    public void onBindViewHolder(PreferenceViewHolder holder) {
        super.onBindViewHolder(holder);

        if (!BuildInfo.isAtLeastQ()) {
            holder.findViewById(R.id.system_default).setVisibility(View.GONE);
        }
    }
}
