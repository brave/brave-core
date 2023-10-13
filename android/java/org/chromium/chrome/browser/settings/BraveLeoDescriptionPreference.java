/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.preference.Preference;
import androidx.preference.PreferenceViewHolder;

import org.chromium.chrome.R;
import org.chromium.components.browser_ui.settings.ChromeBasePreference;

/**
 * The preference used to reset Brave Leo.
 */
public class BraveLeoDescriptionPreference
        extends ChromeBasePreference implements Preference.OnPreferenceClickListener {
    /**
     * Constructor for BraveLeoDescriptionPreference.
     */
    public BraveLeoDescriptionPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    public boolean onPreferenceClick(@NonNull Preference preference) {
        return true;
    }

    @Override
    public void onBindViewHolder(@NonNull PreferenceViewHolder holder) {
        super.onBindViewHolder(holder);
        TextView titleView = (TextView) holder.findViewById(android.R.id.summary);
        assert titleView != null;
        titleView.setTextAppearance(R.style.BraveLeoDescriptionText);
    }
}
