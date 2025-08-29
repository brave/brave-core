/* Copyright (c) 2021 The Brave Authors. All rights reserved.
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

import org.chromium.brave_vpn.mojom.Region;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.vpn.activities.VpnServerSelectionActivity.OnQuickServerSelection;
import org.chromium.chrome.browser.vpn.activities.VpnServerSelectionActivity.OnServerRegionSelection;
import org.chromium.chrome.browser.vpn.utils.BraveVpnPrefUtils;
import org.chromium.chrome.browser.vpn.utils.BraveVpnUtils;

import java.util.ArrayList;
import java.util.List;

public class BraveVpnServerSelectionAdapter
        extends RecyclerView.Adapter<BraveVpnServerSelectionAdapter.ViewHolder> {
    private final Context mContext;
    private List<Region> mRegions = new ArrayList<>();
    private OnServerRegionSelection mOnServerRegionSelection;
    private OnQuickServerSelection mOnQuickServerSelection;
    private Region mActiveRegion;

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

    private static enum CheckedState {
        SET_CHECKED,
        SET_UNCHECKED,
    }

    @Override
    public void onBindViewHolder(
            @NonNull ViewHolder holder, int position, @NonNull List<Object> payloads) {
        if (payloads.isEmpty()) {
            onBindViewHolder(holder, position);
            return;
        }
        Object isCheckedObj = payloads.get(0);
        if (isCheckedObj instanceof CheckedState) {
            boolean isChecked = ((CheckedState) isCheckedObj) == CheckedState.SET_CHECKED;
            holder.serverRadioButton.setChecked(isChecked);
            if (isChecked) {
                mActiveRegion = mRegions.get(position);
            }
        }
    }

    @Override
    public void onBindViewHolder(@NonNull ViewHolder holder, int position) {
        final Region region = mRegions.get(position);
        if (region != null) {
            holder.serverIconText.setText(BraveVpnUtils.countryCodeToEmoji(region.countryIsoCode));
            holder.serverText.setText(region.namePretty);
            String cityText =
                    mContext.getResources()
                            .getQuantityString(
                                    R.plurals.city_text,
                                    region.cities.length,
                                    region.cities.length);
            String serverText =
                    mContext.getResources()
                            .getQuantityString(
                                    R.plurals.server_text, region.serverCount, region.serverCount);
            holder.cityServerText.setText(cityText.concat(serverText));
            boolean isChecked = BraveVpnPrefUtils.getRegionIsoCode().equals(region.countryIsoCode);
            holder.serverRadioButton.setChecked(isChecked);
            if (isChecked) {
                mActiveRegion = region;
            }
            holder.serverRadioButton.setOnClickListener(
                    new View.OnClickListener() {
                        @Override
                        public void onClick(View v) {
                            if (region == mActiveRegion) {
                                return;
                            }
                            mOnQuickServerSelection.onQuickServerSelect(region);
                            if (mActiveRegion != null) {
                                notifyItemChanged(
                                        mRegions.indexOf(mActiveRegion),
                                        CheckedState.SET_UNCHECKED);
                            }
                            notifyItemChanged(
                                    holder.getBindingAdapterPosition(), CheckedState.SET_CHECKED);
                        }
                    });
            holder.serverSelectionItemLayout.setOnClickListener(
                    new View.OnClickListener() {
                        @Override
                        public void onClick(View v) {
                            mOnServerRegionSelection.onServerRegionClick(region);
                        }
                    });
        }
    }

    @Override
    public int getItemCount() {
        return mRegions.size();
    }

    public void setVpnServerRegions(List<Region> vpnServerRegions) {
        this.mRegions = vpnServerRegions;
    }

    public void setOnServerRegionSelection(OnServerRegionSelection onServerRegionSelection) {
        this.mOnServerRegionSelection = onServerRegionSelection;
    }

    public void setOnQuickServerSelection(OnQuickServerSelection onQuickServerSelection) {
        this.mOnQuickServerSelection = onQuickServerSelection;
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
