/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.playlist.settings;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.RadioGroup;

import androidx.preference.Preference;
import androidx.preference.PreferenceViewHolder;

import org.chromium.chrome.R;
import org.chromium.components.browser_ui.widget.RadioButtonWithDescription;
import org.chromium.components.browser_ui.widget.RadioButtonWithDescriptionLayout;

import java.util.ArrayList;
import java.util.Collections;

public class RadioButtonGroupPlaylistAutoSavePreference
        extends Preference implements RadioGroup.OnCheckedChangeListener {
    static final int OPTIONS_SIZE = 3;

    private int mSetting;
    private RadioButtonWithDescription mSettingRadioButton;
    private RadioButtonWithDescriptionLayout mGroup;
    private ArrayList<RadioButtonWithDescription> mButtons;

    public RadioButtonGroupPlaylistAutoSavePreference(Context context, AttributeSet attrs) {
        super(context, attrs);
        setLayoutResource(R.layout.radio_button_group_playlist_auto_save_preference);

        mButtons = new ArrayList<>(Collections.nCopies(OPTIONS_SIZE, null));
    }

    public void initialize(int type) {
        mSetting = type;
    }

    @Override
    public void onBindViewHolder(PreferenceViewHolder holder) {
        super.onBindViewHolder(holder);

        mGroup = (RadioButtonWithDescriptionLayout) holder.findViewById(
                R.id.playlist_auto_save_radio_group);
        mGroup.setOnCheckedChangeListener(this);

        mButtons.set(0,
                (RadioButtonWithDescription) holder.findViewById(R.id.auto_save_on_radio_button));
        // mButtons.set(1,
        //         (RadioButtonWithDescription)
        //         holder.findViewById(R.id.auto_save_off_radio_button));
        mButtons.set(1,
                (RadioButtonWithDescription) holder.findViewById(R.id.auto_save_wifi_radio_button));

        mSettingRadioButton = mButtons.get(mSetting);
        mSettingRadioButton.setChecked(true);
    }

    @Override
    public void onCheckedChanged(RadioGroup group, int checkedId) {
        for (int i = 0; i < OPTIONS_SIZE; i++) {
            if (mButtons.get(i) != null && mButtons.get(i).isChecked()) {
                mSetting = i;
                mSettingRadioButton = mButtons.get(i);
                break;
            }
        }
        assert mSetting >= 0 && mSetting < OPTIONS_SIZE : "No matching setting found.";

        callChangeListener(mSetting);
    }
}
