/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.RadioGroup;

import androidx.preference.Preference;
import androidx.preference.PreferenceViewHolder;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ntp.BraveFreshNtpHelper;
import org.chromium.components.browser_ui.widget.RadioButtonWithDescription;
import org.chromium.components.browser_ui.widget.RadioButtonWithDescriptionLayout;

import java.util.ArrayList;

@NullMarked
public class BraveRadioButtonGroupOpeningScreenPreference extends Preference
        implements RadioGroup.OnCheckedChangeListener {
    private static final int OPTIONS_SIZE = 3;

    // Inactivity hours by variant
    private static final int INACTIVITY_HOURS_VARIANT_B_D = 1;
    private static final int INACTIVITY_HOURS_VARIANT_C = 2;

    private int mSetting;
    @Nullable private RadioButtonWithDescription mSettingRadioButton;
    @Nullable private RadioButtonWithDescriptionLayout mGroup;
    private final ArrayList<@Nullable RadioButtonWithDescription> mButtons;

    public BraveRadioButtonGroupOpeningScreenPreference(
            Context context, @Nullable AttributeSet attrs) {
        super(context, attrs);
        setLayoutResource(R.layout.radio_button_group_opening_screen_preference);

        mButtons = new ArrayList<>(OPTIONS_SIZE);
        for (int i = 0; i < OPTIONS_SIZE; i++) {
            mButtons.add(null);
        }
    }

    public void initialize(int option) {
        mSetting = option;
    }

    @Override
    public void onBindViewHolder(PreferenceViewHolder holder) {
        super.onBindViewHolder(holder);

        mGroup =
                (RadioButtonWithDescriptionLayout)
                        holder.findViewById(R.id.new_opening_screen_radio_group);
        if (mGroup == null) {
            return;
        }
        mGroup.setOnCheckedChangeListener(this);

        mButtons.set(
                BravePreferenceKeys.BRAVE_OPENING_SCREEN_OPTION_LAST_OPEN_TAB,
                (RadioButtonWithDescription)
                        holder.findViewById(R.id.new_opening_screen_last_open_tab_radio_button));
        mButtons.set(
                BravePreferenceKeys.BRAVE_OPENING_SCREEN_OPTION_NEW_TAB_AFTER_INACTIVITY,
                (RadioButtonWithDescription)
                        holder.findViewById(
                                R.id.new_opening_screen_new_tab_after_inactivity_radio_button));
        mButtons.set(
                BravePreferenceKeys.BRAVE_OPENING_SCREEN_OPTION_NEW_TAB,
                (RadioButtonWithDescription)
                        holder.findViewById(R.id.new_opening_screen_new_tab_radio_button));

        // Set dynamic text for inactivity option based on variant
        RadioButtonWithDescription inactivityButton =
                mButtons.get(
                        BravePreferenceKeys.BRAVE_OPENING_SCREEN_OPTION_NEW_TAB_AFTER_INACTIVITY);
        if (inactivityButton != null) {
            String variant = BraveFreshNtpHelper.getVariant();
            int hours = INACTIVITY_HOURS_VARIANT_B_D; // Default for variants B and D
            if (variant != null && variant.equals("C")) {
                hours = INACTIVITY_HOURS_VARIANT_C;
            }
            String hoursText =
                    getContext()
                            .getResources()
                            .getQuantityString(
                                    R.plurals.opening_screen_new_tab_after_inactivity,
                                    hours,
                                    hours);
            inactivityButton.setPrimaryText(hoursText);
        }

        mSettingRadioButton = mButtons.get(mSetting);
        if (mSettingRadioButton != null) {
            mSettingRadioButton.setChecked(true);
        }
    }

    @Override
    public void onCheckedChanged(RadioGroup group, int checkedId) {
        for (int i = 0; i < OPTIONS_SIZE; i++) {
            RadioButtonWithDescription button = mButtons.get(i);
            if (button != null && button.isChecked()) {
                mSetting = i;
                mSettingRadioButton = button;
                break;
            }
        }
        assert mSetting >= 0 && mSetting < OPTIONS_SIZE : "No matching setting found.";

        callChangeListener(mSetting);
    }
}
