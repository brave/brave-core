/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.decentralized_dns.settings;

import android.content.Context;
import android.util.AttributeSet;
import android.util.Pair;
import android.widget.RadioGroup;

import androidx.preference.Preference;
import androidx.preference.PreferenceViewHolder;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.decentralized_dns.EnsOffchainResolveMethod;
import org.chromium.components.browser_ui.widget.RadioButtonWithDescription;
import org.chromium.components.browser_ui.widget.RadioButtonWithDescriptionLayout;

import java.util.ArrayList;

public class RadioButtonGroupEnsOffchainResolveMethodPreference
        extends Preference implements RadioGroup.OnCheckedChangeListener {
    private @EnsOffchainResolveMethod int mSetting;
    private RadioButtonWithDescription mSettingRadioButton;
    private RadioButtonWithDescriptionLayout mGroup;
    private ArrayList<Pair<Integer, RadioButtonWithDescription>> mButtons;

    public RadioButtonGroupEnsOffchainResolveMethodPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
        setLayoutResource(R.layout.radio_button_group_ddns_resolve_method_preference);
    }

    public void initialize(@EnsOffchainResolveMethod int type) {
        mSetting = type;
    }

    @Override
    public void onBindViewHolder(PreferenceViewHolder holder) {
        super.onBindViewHolder(holder);

        mGroup = (RadioButtonWithDescriptionLayout) holder.findViewById(
                R.id.ddns_resolve_method_radio_group);
        mGroup.setOnCheckedChangeListener(this);

        mButtons = new ArrayList<>();
        mButtons.add(Pair.create(EnsOffchainResolveMethod.ASK,
                (RadioButtonWithDescription) holder.findViewById(
                        R.id.ddns_resolve_method_ask_radio_button)));
        mButtons.add(Pair.create(EnsOffchainResolveMethod.DISABLED,
                (RadioButtonWithDescription) holder.findViewById(
                        R.id.ddns_resolve_method_disabled_radio_button)));
        mButtons.add(Pair.create(EnsOffchainResolveMethod.ENABLED,
                (RadioButtonWithDescription) holder.findViewById(
                        R.id.ddns_resolve_method_enabled_radio_button)));

        for (int i = 0; i < mButtons.size(); i++) {
            if (mButtons.get(i).first == mSetting) {
                mSettingRadioButton = mButtons.get(i).second;
                mSettingRadioButton.setChecked(true);
                break;
            }
        }
    }

    @Override
    public void onCheckedChanged(RadioGroup group, int checkedId) {
        for (int i = 0; i < mButtons.size(); i++) {
            if (mButtons.get(i).second.isChecked()) {
                mSetting = mButtons.get(i).first;
                mSettingRadioButton = mButtons.get(i).second;
                break;
            }
        }
        callChangeListener(mSetting);
    }
}
