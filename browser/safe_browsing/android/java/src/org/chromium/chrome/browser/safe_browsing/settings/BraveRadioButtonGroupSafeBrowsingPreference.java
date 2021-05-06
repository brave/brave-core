/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.safe_browsing.settings;

import android.content.Context;
import android.util.AttributeSet;
import android.view.View;

import androidx.preference.PreferenceViewHolder;

import org.chromium.components.browser_ui.widget.RadioButtonWithDescriptionAndAuxButton;

public class BraveRadioButtonGroupSafeBrowsingPreference
        extends RadioButtonGroupSafeBrowsingPreference {
    private RadioButtonWithDescriptionAndAuxButton mEnhancedProtection;

    public BraveRadioButtonGroupSafeBrowsingPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    public void onBindViewHolder(PreferenceViewHolder holder) {
        super.onBindViewHolder(holder);
        RadioButtonWithDescriptionAndAuxButton enhancedProtection =
                (RadioButtonWithDescriptionAndAuxButton) holder.findViewById(
                        R.id.enhanced_protection);
        assert enhancedProtection != null : "Something has changed in the upstream!";
        if (enhancedProtection != null) {
            enhancedProtection.setVisibility(View.GONE);
        }
    }
}
