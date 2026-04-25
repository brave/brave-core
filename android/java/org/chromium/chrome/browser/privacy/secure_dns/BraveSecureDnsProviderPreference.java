// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

package org.chromium.chrome.browser.privacy.secure_dns;

import android.content.Context;
import android.util.AttributeSet;

import androidx.preference.PreferenceViewHolder;

import org.chromium.base.BraveFeatureList;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.components.browser_ui.widget.RadioButtonWithDescription;

class BraveSecureDnsProviderPreference extends SecureDnsProviderPreference {
    private String mPrimaryText;
    private String mDescriptionText;

    public BraveSecureDnsProviderPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
        if (ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_FALLBACK_DOH_PROVIDER)) {
            String endpoint =
                    ChromeFeatureList.getFieldTrialParamByFeature(
                            BraveFeatureList.BRAVE_FALLBACK_DOH_PROVIDER,
                            "BraveFallbackDoHProviderEndpoint");
            String altDescriptionText =
                    context.getString(R.string.settings_automatic_mode_description_fallback);
            switch (endpoint) {
                case "quad9":
                    mPrimaryText =
                            context.getString(R.string.settings_automatic_mode_with_quad9_label);
                    mDescriptionText = altDescriptionText;
                    break;
                case "wikimedia":
                    mPrimaryText =
                            context.getString(
                                    R.string.settings_automatic_mode_with_wikimedia_label);
                    mDescriptionText = altDescriptionText;
                    break;
                case "cloudflare":
                    mPrimaryText =
                            context.getString(
                                    R.string.settings_automatic_mode_with_cloudflare_label);
                    mDescriptionText = altDescriptionText;
                    break;
                case "none":
                default:
                    mPrimaryText = context.getString(R.string.settings_automatic_mode_label);
                    break;
            }
        }
    }

    @Override
    public void onBindViewHolder(PreferenceViewHolder holder) {
        super.onBindViewHolder(holder);
        if (ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_FALLBACK_DOH_PROVIDER)) {
            RadioButtonWithDescription automaticModeButton =
                    (RadioButtonWithDescription) holder.findViewById(R.id.automatic);
            automaticModeButton.setPrimaryText(mPrimaryText);
            automaticModeButton.setDescriptionText(mDescriptionText);
        }
    }
}
