/**
 * Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

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

import org.chromium.chrome.R;
import org.chromium.chrome.browser.vpn.activities.VpnServerSelectionActivity.OnServerRegionSelection;
import org.chromium.chrome.browser.vpn.models.BraveVpnServerRegion;
import org.chromium.chrome.browser.vpn.utils.BraveVpnPrefUtils;
import org.chromium.chrome.browser.vpn.utils.BraveVpnUtils;

import java.util.ArrayList;
import java.util.List;

public class BraveVpnServerSelectionAdapter
        extends RecyclerView.Adapter<BraveVpnServerSelectionAdapter.ViewHolder> {
    private final Context mContext;
    private List<BraveVpnServerRegion> mBraveVpnServerRegions = new ArrayList<>();
    private OnServerRegionSelection mOnServerRegionSelection;

    public BraveVpnServerSelectionAdapter(Context context) {
        this.mContext = context;
    }

    @NonNull
    @Override
    public ViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        LayoutInflater layoutInflater = LayoutInflater.from(parent.getContext());
        View listItem = layoutInflater.inflate(
                R.layout.brave_vpn_server_selection_item_layout, parent, false);
        return new ViewHolder(listItem);
    }

    @Override
    public void onBindViewHolder(@NonNull ViewHolder holder, int position) {
        final BraveVpnServerRegion vpnServerRegion = mBraveVpnServerRegions.get(position);
        if (vpnServerRegion != null) {
            holder.serverIconText.setText(
                    BraveVpnUtils.countryCodeToEmoji(vpnServerRegion.getCountryIsoCode()));
            holder.serverText.setText(vpnServerRegion.getNamePretty());
            holder.cityServerText.setText(
                    mContext.getResources().getString(R.string.city_server_text));
            holder.serverRadioButton.setChecked(
                    BraveVpnPrefUtils.getServerRegion().equals(vpnServerRegion.getName()));
            // if (BraveVpnPrefUtils.getServerRegion().equals(vpnServerRegion.getName())) {
            //     holder.serverText.setCompoundDrawablesWithIntrinsicBounds(
            //             0, 0, R.drawable.ic_checkbox_checked, 0);
            // } else {
            //     holder.serverText.setCompoundDrawablesWithIntrinsicBounds(0, 0, 0, 0);
            // }
            holder.serverSelectionItemLayout.setOnClickListener(
                    new View.OnClickListener() {
                        @Override
                        public void onClick(View v) {
                            mOnServerRegionSelection.onServerRegionClick(vpnServerRegion);
                        }
                    });
        }
    }

    @Override
    public int getItemCount() {
        return mBraveVpnServerRegions.size();
    }

    public void setVpnServerRegions(List<BraveVpnServerRegion> vpnServerRegions) {
        this.mBraveVpnServerRegions = vpnServerRegions;
    }

    public void setOnServerRegionSelection(OnServerRegionSelection onServerRegionSelection) {
        this.mOnServerRegionSelection = onServerRegionSelection;
    }

    public static class ViewHolder extends RecyclerView.ViewHolder {
        public LinearLayout serverSelectionItemLayout;
        public TextView serverIconText;
        public TextView serverText;
        public TextView cityServerText;
        public MaterialRadioButton serverRadioButton;

        public ViewHolder(View itemView) {
            super(itemView);
            this.serverSelectionItemLayout =
                    itemView.findViewById(R.id.server_selection_item_layout);
            this.serverIconText = itemView.findViewById(R.id.server_icon);
            this.serverText = itemView.findViewById(R.id.server_text);
            this.cityServerText = itemView.findViewById(R.id.city_server_text);
            this.serverRadioButton = itemView.findViewById(R.id.server_radio_button);
        }
    }
}
