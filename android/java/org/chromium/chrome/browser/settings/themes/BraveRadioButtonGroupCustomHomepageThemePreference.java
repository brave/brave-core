/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings.themes;

import android.content.Context;
import android.support.v7.preference.PreferenceViewHolder;
import android.util.AttributeSet;
import android.view.View;
import android.widget.RadioGroup;

import org.chromium.chrome.R;
import org.chromium.base.BuildInfo;
import org.chromium.base.Log;
import org.chromium.components.browser_ui.widget.RadioButtonWithDescription;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.ntp_background_images.NTPBackgroundImagesBridge;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;
import org.chromium.chrome.browser.preferences.BravePref;

public class BraveRadioButtonGroupCustomHomepageThemePreference extends RadioButtonGroupThemePreference {

    private RadioButtonWithDescription braveDefaultView, refView;
    private NTPBackgroundImagesBridge mNTPBackgroundImagesBridge;

    public BraveRadioButtonGroupCustomHomepageThemePreference(Context context, AttributeSet attrs) {
        super(context, attrs);
        Profile mProfile = Profile.getLastUsedProfile();
        mNTPBackgroundImagesBridge = NTPBackgroundImagesBridge.getInstance(mProfile);
    }

    @Override
    public void onBindViewHolder(PreferenceViewHolder holder) {
        super.onBindViewHolder(holder);

        if (!BuildInfo.isAtLeastQ()) {
            holder.findViewById(R.id.system_default).setVisibility(View.GONE);
        }

        RadioButtonWithDescription braveDefaultView = (RadioButtonWithDescription)holder.findViewById(R.id.light);
        RadioButtonWithDescription refView = (RadioButtonWithDescription)holder.findViewById(R.id.dark);
        if (mNTPBackgroundImagesBridge != null && mNTPBackgroundImagesBridge.isSuperReferral()) {
            refView.setPrimaryText(mNTPBackgroundImagesBridge.getSuperReferralThemeName());
            if(BravePrefServiceBridge.getInstance().getInteger(BravePref.NTP_SHOW_SUPER_REFERRAL_THEMES_OPTION) == 1 ? true : false) {
                refView.setChecked(true);
                braveDefaultView.setChecked(false);
            } else {
                refView.setChecked(false);
                braveDefaultView.setChecked(true);
            }
        }
        braveDefaultView.setPrimaryText(getContext().getResources().getString(R.string.brave_default));
    }

    @Override
    public void onCheckedChanged(RadioGroup group, int checkedId) {
        super.onCheckedChanged(group, checkedId);
        BravePrefServiceBridge.getInstance().setInteger(BravePref.NTP_SHOW_SUPER_REFERRAL_THEMES_OPTION, checkedId == R.id.light ? 0 : 1 );
    }
}
