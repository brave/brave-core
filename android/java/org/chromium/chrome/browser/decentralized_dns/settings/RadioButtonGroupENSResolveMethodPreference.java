/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.decentralized_dns.settings;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.RadioGroup;

import androidx.preference.Preference;
import androidx.preference.PreferenceViewHolder;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.decentralized_dns.ResolveMethodTypes;
import org.chromium.components.browser_ui.widget.RadioButtonWithDescription;
import org.chromium.components.browser_ui.widget.RadioButtonWithDescriptionLayout;

import java.util.ArrayList;
import java.util.Collections;

public class RadioButtonGroupENSResolveMethodPreference
        extends Preference implements RadioGroup.OnCheckedChangeListener {
    static final int OPTIONS_SIZE = ResolveMethodTypes.MAX_VALUE + 1;

    private @ResolveMethodTypes int mSetting;
    private RadioButtonWithDescription mSettingRadioButton;
    private RadioButtonWithDescriptionLayout mGroup;
    private ArrayList<RadioButtonWithDescription> mButtons;

    public RadioButtonGroupENSResolveMethodPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
        setLayoutResource(R.layout.radio_button_group_ens_resolve_method_preference);

        mButtons = new ArrayList<>(Collections.nCopies(OPTIONS_SIZE, null));
    }

    public void initialize(@ResolveMethodTypes int type) {
        mSetting = type;
    }

    @Override
    public void onBindViewHolder(PreferenceViewHolder holder) {
        super.onBindViewHolder(holder);

        mGroup = (RadioButtonWithDescriptionLayout) holder.findViewById(
                R.id.ens_resolve_method_radio_group);
        mGroup.setOnCheckedChangeListener(this);

        mButtons.set(ResolveMethodTypes.ASK,
                (RadioButtonWithDescription) holder.findViewById(
                        R.id.ens_resolve_method_ask_radio_button));
        mButtons.set(ResolveMethodTypes.DISABLED,
                (RadioButtonWithDescription) holder.findViewById(
                        R.id.ens_resolve_method_disabled_radio_button));
        mButtons.set(ResolveMethodTypes.ETHEREUM,
                (RadioButtonWithDescription) holder.findViewById(
                        R.id.ens_resolve_method_ethereum_radio_button));

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
