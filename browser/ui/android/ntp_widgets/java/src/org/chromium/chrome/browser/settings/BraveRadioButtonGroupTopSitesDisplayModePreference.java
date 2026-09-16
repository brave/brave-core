/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.RadioGroup;

import androidx.preference.Preference;
import androidx.preference.PreferenceViewHolder;

import org.chromium.brave.browser.ntp_widgets.R;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.ntp.NtpUtil;
import org.chromium.components.browser_ui.widget.RadioButtonWithDescription;
import org.chromium.components.browser_ui.widget.RadioButtonWithDescriptionLayout;

import java.util.ArrayList;

/**
 * Radio button pair for choosing what the NTP top-sites widget shows, mirroring the modes available
 * from the widget's own long-press menu.
 */
@NullMarked
public class BraveRadioButtonGroupTopSitesDisplayModePreference extends Preference
        implements RadioGroup.OnCheckedChangeListener {
    private static final int OPTIONS_SIZE = 2;

    private int mSetting;
    @Nullable private RadioButtonWithDescriptionLayout mGroup;
    private final ArrayList<@Nullable RadioButtonWithDescription> mButtons;

    public BraveRadioButtonGroupTopSitesDisplayModePreference(
            Context context, @Nullable AttributeSet attrs) {
        super(context, attrs);
        setLayoutResource(R.layout.radio_button_group_top_sites_display_mode_preference);

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
                        holder.findViewById(R.id.top_sites_display_mode_radio_group);
        if (mGroup == null) {
            return;
        }
        mGroup.setOnCheckedChangeListener(this);

        mButtons.set(
                NtpUtil.TOP_SITES_MODE_SHORTCUTS,
                (RadioButtonWithDescription)
                        holder.findViewById(R.id.top_sites_display_mode_favorites_radio_button));
        mButtons.set(
                NtpUtil.TOP_SITES_MODE_FREQUENT,
                (RadioButtonWithDescription)
                        holder.findViewById(R.id.top_sites_display_mode_frequent_radio_button));

        RadioButtonWithDescription settingRadioButton = mButtons.get(mSetting);
        if (settingRadioButton != null) {
            settingRadioButton.setChecked(true);
        }
    }

    @Override
    public void onCheckedChanged(RadioGroup group, int checkedId) {
        for (int i = 0; i < OPTIONS_SIZE; i++) {
            RadioButtonWithDescription button = mButtons.get(i);
            if (button != null && button.isChecked()) {
                mSetting = i;
                break;
            }
        }
        assert mSetting >= 0 && mSetting < OPTIONS_SIZE : "No matching setting found.";

        callChangeListener(mSetting);
    }
}
