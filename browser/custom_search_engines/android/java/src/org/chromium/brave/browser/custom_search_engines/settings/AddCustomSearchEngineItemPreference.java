/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.custom_search_engines.settings;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.TextView;

import androidx.preference.PreferenceViewHolder;

import org.chromium.brave.browser.custom_search_engines.R;
import org.chromium.build.annotations.NullMarked;
import org.chromium.components.browser_ui.settings.ChromeBasePreference;

/** A preference that navigates to {@link AddCustomSearchEnginePreferenceFragment}. */
@NullMarked
public class AddCustomSearchEngineItemPreference extends ChromeBasePreference {
    public AddCustomSearchEngineItemPreference(Context context) {
        super(context);
    }

    public AddCustomSearchEngineItemPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    public void onBindViewHolder(PreferenceViewHolder holder) {
        super.onBindViewHolder(holder);

        if (holder.findViewById(android.R.id.title) instanceof TextView titleView) {
            titleView.setTextAppearance(R.style.AddCustomSearchEngineItemTitle);
        }
    }
}
