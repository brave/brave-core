// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

package org.chromium.chrome.browser.settings;

import android.content.Context;
import android.util.AttributeSet;
import android.util.TypedValue;
import android.view.View;
import android.view.ViewGroup;
import android.widget.RadioGroup;

import androidx.preference.Preference;
import androidx.preference.PreferenceViewHolder;

import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.preferences.website.BraveShieldsContentSettings;
import org.chromium.components.browser_ui.widget.RadioButtonWithDescription;
import org.chromium.components.browser_ui.widget.RadioButtonWithDescriptionLayout;

import java.util.HashMap;
import java.util.Map;

@NullMarked
public class BraveShredPreference extends Preference implements RadioGroup.OnCheckedChangeListener {
    private final HashMap<String, RadioButtonWithDescription> mButtons;
    private RadioButtonWithDescriptionLayout mGroup;
    private RadioButtonWithDescription mSettingRadioButton;
    private String mSetting;

    public BraveShredPreference(Context context, AttributeSet attrs) {
        super(context, attrs);

        setLayoutResource(R.layout.brave_shred_preference);
        mButtons = new HashMap<>();
        mButtons.put(BraveShieldsContentSettings.AUTO_SHRED_MODE_NEVER, null);
        mButtons.put(BraveShieldsContentSettings.AUTO_SHRED_MODE_LAST_TAB_CLOSED, null);
        mButtons.put(BraveShieldsContentSettings.AUTO_SHRED_MODE_APP_EXIT, null);
    }

    public void initialize(String autoShredPrefs) {
        mSetting = autoShredPrefs;
    }

    @Override
    public void onBindViewHolder(PreferenceViewHolder holder) {
        super.onBindViewHolder(holder);

        mGroup = (RadioButtonWithDescriptionLayout) holder.findViewById(R.id.radio_button_layout);
        mGroup.setOnCheckedChangeListener(this);
        View linearLayout = (View) mGroup.getParent();
        ViewGroup.MarginLayoutParams params =
                (ViewGroup.MarginLayoutParams) linearLayout.getLayoutParams();
        params.bottomMargin =
                (int)
                        TypedValue.applyDimension(
                                TypedValue.COMPLEX_UNIT_DIP,
                                8,
                                linearLayout.getResources().getDisplayMetrics());
        linearLayout.setLayoutParams(params);

        mButtons.put(
                BraveShieldsContentSettings.AUTO_SHRED_MODE_NEVER,
                (RadioButtonWithDescription) holder.findViewById(R.id.auto_shred_never_mode));
        mButtons.put(
                BraveShieldsContentSettings.AUTO_SHRED_MODE_LAST_TAB_CLOSED,
                (RadioButtonWithDescription)
                        holder.findViewById(R.id.auto_shred_site_tab_closed_mode));
        mButtons.put(
                BraveShieldsContentSettings.AUTO_SHRED_MODE_APP_EXIT,
                (RadioButtonWithDescription) holder.findViewById(R.id.auto_shred_app_close_mode));
        mSettingRadioButton = mButtons.get(mSetting);
        mSettingRadioButton.setChecked(true);
    }

    @Override
    public void onCheckedChanged(RadioGroup group, int checkedId) {
        for (Map.Entry<String, RadioButtonWithDescription> entry : mButtons.entrySet()) {
            String key = entry.getKey();
            RadioButtonWithDescription button = entry.getValue();
            if (button != null && button.isChecked()) {
                mSetting = key;
                mSettingRadioButton = button;
                callChangeListener(mSetting);
                break;
            }
        }
    }
}
