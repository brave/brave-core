/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.vpn.settings;

import android.content.Context;
import android.util.AttributeSet;
import android.view.View;
import android.widget.TextView;

import androidx.preference.Preference;
import androidx.preference.PreferenceViewHolder;

import org.chromium.brave_vpn.mojom.BraveVpnConstants;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.vpn.utils.BraveVpnPrefUtils;
import org.chromium.chrome.browser.vpn.utils.BraveVpnUtils;

public class VpnRegionPreference extends Preference {
    private Context mContext;

    public VpnRegionPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
        mContext = context;
    }

    @Override
    public void onBindViewHolder(PreferenceViewHolder holder) {
        super.onBindViewHolder(holder);

        TextView regionFlag = (TextView) holder.findViewById(R.id.region_flag);
        if (!BraveVpnPrefUtils.getRegionIsoCode().isEmpty()) {
            regionFlag.setText(
                    BraveVpnUtils.countryCodeToEmoji(BraveVpnPrefUtils.getRegionIsoCode()));
        }

        String serverLocationTitle = BraveVpnPrefUtils.getRegionCountry();
        String optimalString = "%s - %s";
        String serverLocationSummary =
                BraveVpnPrefUtils.getRegionPrecision()
                                .equals(BraveVpnConstants.REGION_PRECISION_COUNTRY)
                        ? String.format(
                                optimalString,
                                mContext.getString(R.string.optimal_text),
                                BraveVpnPrefUtils.getHostnameDisplay())
                        : BraveVpnPrefUtils.getHostnameDisplay();

        TextView regionTitle = (TextView) holder.findViewById(R.id.region_title);
        regionTitle.setText(serverLocationTitle);
        TextView regionSummary = (TextView) holder.findViewById(R.id.region_summary);
        regionSummary.setText(serverLocationSummary);

        holder.itemView.setOnClickListener(
                new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        BraveVpnUtils.openVpnServerSelectionActivity(mContext);
                    }
                });
    }
}
