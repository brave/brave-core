/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.components.browser_ui.site_settings;

import android.content.Context;
import android.util.AttributeSet;

import androidx.preference.Preference;
import androidx.preference.PreferenceViewHolder;

import org.chromium.components.browser_ui.widget.RadioButtonWithDescription;

public class BraveFourStateCookieSettingsPreferenceBase extends Preference {
    private Context mContext;
    private RadioButtonWithDescription mBlockThirdPartyIncognitoButton;

    public BraveFourStateCookieSettingsPreferenceBase(Context context, AttributeSet attrs) {
        super(context, attrs);

        mContext = context;
    }

    @Override
    public void onBindViewHolder(PreferenceViewHolder holder) {
        super.onBindViewHolder(holder);

        mBlockThirdPartyIncognitoButton =
                (RadioButtonWithDescription) holder.findViewById(R.id.block_third_party_incognito);
        assert mBlockThirdPartyIncognitoButton != null : "Something has changed in the upstream!";
        if (mBlockThirdPartyIncognitoButton != null) {
            mBlockThirdPartyIncognitoButton.setPrimaryText(mContext.getString(
                    R.string.website_settings_category_cookie_block_third_party_private_title));
        }
    }
}
