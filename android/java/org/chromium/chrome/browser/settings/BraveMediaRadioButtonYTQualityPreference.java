/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.content.Context;
import android.util.AttributeSet;

import androidx.preference.Preference;
import androidx.preference.PreferenceViewHolder;

import org.chromium.chrome.R;
import org.chromium.components.browser_ui.widget.RadioButtonWithDescription;
import org.chromium.components.browser_ui.widget.RadioButtonWithDescriptionLayout;

public class BraveMediaRadioButtonYTQualityPreference extends Preference {

    /** This is a delegate that is implemented in BraveMediaYTQualityPreferences */
    public interface RadioButtonsDelegate {
        default void setDefaultQuality(@YTVideoQuality int quality) {}
    }

    private @YTVideoQuality int mQuality;
    private RadioButtonsDelegate mDelegate;

    public BraveMediaRadioButtonYTQualityPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
        setLayoutResource(R.xml.brave_media_radio_button_yt_quality_preference);
    }

    public void initialize(RadioButtonsDelegate delegate, @YTVideoQuality int quality) {
        mDelegate = delegate;
        mQuality = quality;
    }

    @Override
    public void onBindViewHolder(PreferenceViewHolder holder) {
        super.onBindViewHolder(holder);

        RadioButtonWithDescriptionLayout group =
                (RadioButtonWithDescriptionLayout) holder.findViewById(R.id.radio_button_layout);
        RadioButtonWithDescription rbOn = (RadioButtonWithDescription) holder.findViewById(R.id.on);
        RadioButtonWithDescription rbAllowWifi =
                (RadioButtonWithDescription) holder.findViewById(R.id.allow_over_wifi);
        RadioButtonWithDescription rbOff =
                (RadioButtonWithDescription) holder.findViewById(R.id.off);
        if (mQuality == YTVideoQuality.ON) {
            rbOn.setChecked(true);
        } else if (mQuality == YTVideoQuality.ALLOW_OVER_WIFI) {
            rbAllowWifi.setChecked(true);
        } else if (mQuality == YTVideoQuality.OFF) {
            rbOff.setChecked(true);
        }
        group.setOnCheckedChangeListener(
                (radioGroup, checkedId) -> {
                    RadioButtonWithDescription checkedRadioButton =
                            (RadioButtonWithDescription) radioGroup.findViewById(checkedId);
                    boolean isChecked = checkedRadioButton.isChecked();
                    if (isChecked) {
                        int qualityPrefs = -1;
                        if (checkedId == R.id.on) {
                            qualityPrefs = YTVideoQuality.ON;
                        } else if (checkedId == R.id.allow_over_wifi) {
                            qualityPrefs = YTVideoQuality.ALLOW_OVER_WIFI;
                        } else if (checkedId == R.id.off) {
                            qualityPrefs = YTVideoQuality.OFF;
                        }

                        if (qualityPrefs != -1) {
                            mDelegate.setDefaultQuality(qualityPrefs);
                        }
                    }
                });
    }
}
