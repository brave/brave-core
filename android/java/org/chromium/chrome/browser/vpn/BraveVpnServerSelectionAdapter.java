/**
 * Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.vpn;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.settings.BraveVpnServerSelectionPreferences.OnServerRegionSelection;
import org.chromium.chrome.browser.vpn.BraveVpnPrefUtils;
import org.chromium.chrome.browser.vpn.BraveVpnServerRegion;

import java.util.ArrayList;
import java.util.List;

public class BraveVpnServerSelectionAdapter
        extends RecyclerView.Adapter<BraveVpnServerSelectionAdapter.ViewHolder> {
    List<BraveVpnServerRegion> mBraveVpnServerRegions = new ArrayList<>();
    private OnServerRegionSelection mOnServerRegionSelection;

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
            holder.serverText.setText(vpnServerRegion.getNamePretty());
            if (BraveVpnPrefUtils.getServerRegion().equals(vpnServerRegion.getName())) {
                holder.serverText.setCompoundDrawablesWithIntrinsicBounds(
                        0, 0, R.drawable.ic_server_selection_check, 0);
            } else {
                holder.serverText.setCompoundDrawablesWithIntrinsicBounds(0, 0, 0, 0);
            }
            holder.serverText.setOnClickListener(new View.OnClickListener() {
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
        public TextView serverText;

        public ViewHolder(View itemView) {
            super(itemView);
            this.serverText = itemView.findViewById(R.id.server_text);
        }
    }
}
