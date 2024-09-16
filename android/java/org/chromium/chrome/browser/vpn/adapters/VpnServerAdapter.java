/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.vpn.adapters;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import com.google.android.material.radiobutton.MaterialRadioButton;

import org.chromium.brave_vpn.mojom.BraveVpnConstants;
import org.chromium.brave_vpn.mojom.Region;
import org.chromium.chrome.R;
import org.chromium.base.Log;
import org.chromium.chrome.browser.vpn.activities.VpnServerActivity.OnCitySelection;
import org.chromium.chrome.browser.vpn.utils.BraveVpnPrefUtils;

import java.util.ArrayList;
import java.util.List;

public class VpnServerAdapter extends RecyclerView.Adapter<VpnServerAdapter.ViewHolder> {
    private final Context mContext;
    private List<Region> mCities = new ArrayList<>();
    private OnCitySelection mOnCitySelection;
    private Region mRegion;

    public VpnServerAdapter(Context context) {
        this.mContext = context;
    }

    @NonNull
    @Override
    public ViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        LayoutInflater layoutInflater = LayoutInflater.from(parent.getContext());
        View listItem = layoutInflater.inflate(R.layout.vpn_server_item_layout, parent, false);
        return new ViewHolder(listItem);
    }

    @Override
    public void onBindViewHolder(@NonNull ViewHolder holder, int position) {
        final Region city = mCities.get(position);
        if (city != null) {
            holder.serverText.setText(city.namePretty);
            String cityServerText =
                    mContext.getResources()
                            .getQuantityString(
                                    R.plurals.server_text, city.serverCount, city.serverCount);
            holder.cityServerText.setText((position == 0) ? city.name : cityServerText);

            Log.e(
                    "brave_vpn",
                    "BraveVpnPrefUtils.getRegionName() : " + BraveVpnPrefUtils.getRegionName());
            Log.e("brave_vpn", "mRegion.name : " + mRegion.name);

            boolean isCountryPrecision =
                    BraveVpnPrefUtils.getRegionPrecision()
                            .equals(BraveVpnConstants.REGION_PRECISION_COUNTRY);
            boolean isCityPrecision =
                    BraveVpnPrefUtils.getRegionPrecision()
                            .equals(BraveVpnConstants.REGION_PRECISION_CITY);
            boolean isRegionMatch =
                    BraveVpnPrefUtils.getRegionName().equals(mRegion.name) && position == 0;
            boolean isCityMatch = BraveVpnPrefUtils.getRegionName().equals(city.name);

            boolean isEnabled =
                    (isCountryPrecision && isRegionMatch) || (isCityPrecision && isCityMatch);

            holder.serverRadioButton.setChecked(isEnabled);
            holder.serverSelectionItemLayout.setOnClickListener(
                    new View.OnClickListener() {
                        @Override
                        public void onClick(View v) {
                            mOnCitySelection.onCityClick(city, holder.getAdapterPosition());
                        }
                    });
        }
    }

    @Override
    public int getItemCount() {
        return mCities.size();
    }

    public void setRegion(Region region) {
        this.mRegion = region;
    }

    public void setCities(List<Region> cities) {
        this.mCities = cities;
    }

    public void setOnCitySelection(OnCitySelection onCitySelection) {
        this.mOnCitySelection = onCitySelection;
    }

    public static class ViewHolder extends RecyclerView.ViewHolder {
        public LinearLayout serverSelectionItemLayout;
        public TextView serverText;
        public TextView cityServerText;
        public MaterialRadioButton serverRadioButton;

        public ViewHolder(View itemView) {
            super(itemView);
            this.serverSelectionItemLayout =
                    itemView.findViewById(R.id.server_selection_item_layout);
            this.serverText = itemView.findViewById(R.id.server_text);
            this.cityServerText = itemView.findViewById(R.id.city_server_text);
            this.serverRadioButton = itemView.findViewById(R.id.server_radio_button);
        }
    }
}
