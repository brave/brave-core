/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.provider.Browser;
import android.util.AttributeSet;
import android.view.View;
import android.widget.RadioGroup;
import android.widget.TextView;

import androidx.annotation.IntDef;
import androidx.preference.Preference;
import androidx.preference.PreferenceViewHolder;

import org.chromium.chrome.R;
import org.chromium.components.browser_ui.widget.RadioButtonWithDescription;
import org.chromium.components.browser_ui.widget.RadioButtonWithDescriptionLayout;

import java.util.ArrayList;
import java.util.Collections;

public class BraveWebrtcPolicyPreference
        extends Preference implements RadioGroup.OnCheckedChangeListener {
    @IntDef({WebrtcPolicy.DEFAULT, WebrtcPolicy.DEFAULT_PUBLIC_AND_PRIVATE_INTERFACES,
            WebrtcPolicy.DEFAULT_PUBLIC_INTERFACE_ONLY, WebrtcPolicy.DISABLE_NON_PROXIED_UDP})
    public @interface WebrtcPolicy {
        int DEFAULT = 0;
        int DEFAULT_PUBLIC_AND_PRIVATE_INTERFACES = 1;
        int DEFAULT_PUBLIC_INTERFACE_ONLY = 2;
        int DISABLE_NON_PROXIED_UDP = 3;

        int NUM_ENTRIES = 4;
    }

    private @WebrtcPolicy int mSetting;
    private RadioButtonWithDescription mSettingRadioButton;
    private RadioButtonWithDescriptionLayout mGroup;
    private TextView mLearnMore;
    private ArrayList<RadioButtonWithDescription> mButtons;
    protected static final String FALLBACK_SUPPORT_URL =
            "https://support.brave.com/hc/en-us/articles/360017989132-How-do-I-change-my-Privacy-Settings-#webrtc";

    @SuppressLint("WrongConstant")
    public BraveWebrtcPolicyPreference(Context context, AttributeSet attrs) {
        super(context, attrs);

        setLayoutResource(R.layout.brave_webrtc_policy_preference);

        mButtons = new ArrayList<>(Collections.nCopies(WebrtcPolicy.NUM_ENTRIES, null));
    }

    public void initialize(@WebrtcPolicy int policy) {
        mSetting = policy;
    }

    @Override
    public void onBindViewHolder(PreferenceViewHolder holder) {
        super.onBindViewHolder(holder);

        mLearnMore = (TextView) holder.findViewById(R.id.learn_more_webrtc);
        mGroup = (RadioButtonWithDescriptionLayout) holder.findViewById(R.id.radio_button_layout);
        mGroup.setOnCheckedChangeListener(this);

        mButtons.set(WebrtcPolicy.DEFAULT,
                (RadioButtonWithDescription) holder.findViewById(R.id.webrtc_policy_default));
        mButtons.set(WebrtcPolicy.DEFAULT_PUBLIC_AND_PRIVATE_INTERFACES,
                (RadioButtonWithDescription) holder.findViewById(
                        R.id.webrtc_policy_default_public_and_private_interfaces));
        mButtons.set(WebrtcPolicy.DEFAULT_PUBLIC_INTERFACE_ONLY,
                (RadioButtonWithDescription) holder.findViewById(
                        R.id.webrtc_policy_default_public_interface_only));
        mButtons.set(WebrtcPolicy.DISABLE_NON_PROXIED_UDP,
                (RadioButtonWithDescription) holder.findViewById(
                        R.id.webrtc_policy_disable_non_proxied_udp));

        mSettingRadioButton = mButtons.get(mSetting);
        mSettingRadioButton.setChecked(true);

        mLearnMore.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse(FALLBACK_SUPPORT_URL));
                // Let Chrome know that this intent is from Chrome, so that it does not close the
                // app when the user presses 'back' button.
                intent.putExtra(Browser.EXTRA_APPLICATION_ID, getContext().getPackageName());
                intent.putExtra(Browser.EXTRA_CREATE_NEW_TAB, true);
                intent.setPackage(getContext().getPackageName());
                getContext().startActivity(intent);
            }
        });
    }

    @Override
    @SuppressLint("WrongConstant")
    public void onCheckedChanged(RadioGroup group, int checkedId) {
        for (int i = 0; i < WebrtcPolicy.NUM_ENTRIES; i++) {
            if (mButtons.get(i).isChecked()) {
                mSetting = i;
                mSettingRadioButton = mButtons.get(i);
                break;
            }
        }
        assert mSetting >= 0 && mSetting < WebrtcPolicy.NUM_ENTRIES : "No matching setting found.";

        callChangeListener(mSetting);
    }
}
