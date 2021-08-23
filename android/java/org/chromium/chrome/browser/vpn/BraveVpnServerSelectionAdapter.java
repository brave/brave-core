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
import org.chromium.chrome.browser.vpn.BraveVpnUtils;

import java.util.ArrayList;
import java.util.List;

public class BraveVpnServerSelectionAdapter
        extends RecyclerView.Adapter<BraveVpnServerSelectionAdapter.ViewHolder> {
    List<VpnServerRegion> vpnServerRegions = new ArrayList<>();
    private OnServerRegionSelection onServerRegionSelection;
    public static final String PREF_SERVER_CHANGE_LOCATION = "server_change_location";

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
        final VpnServerRegion vpnServerRegion = vpnServerRegions.get(position);
        if (vpnServerRegion != null) {
            holder.serverText.setText(vpnServerRegion.getNamePretty());
            if (BraveVpnUtils.getServerRegion(PREF_SERVER_CHANGE_LOCATION)
                            .equals(vpnServerRegion.getName())) {
                holder.serverText.setCompoundDrawablesWithIntrinsicBounds(
                        0, 0, R.drawable.ic_server_selection_check, 0);
            } else {
                holder.serverText.setCompoundDrawablesWithIntrinsicBounds(0, 0, 0, 0);
            }
            holder.serverText.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    onServerRegionSelection.onServerRegionClick(vpnServerRegion);
                }
            });
        }
    }

    @Override
    public int getItemCount() {
        return vpnServerRegions.size();
    }

    public void setVpnServerRegions(List<VpnServerRegion> vpnServerRegions) {
        this.vpnServerRegions = vpnServerRegions;
    }

    public void setOnServerRegionSelection(OnServerRegionSelection onServerRegionSelection) {
        this.onServerRegionSelection = onServerRegionSelection;
    }

    public static class ViewHolder extends RecyclerView.ViewHolder {
        public TextView serverText;

        public ViewHolder(View itemView) {
            super(itemView);
            this.serverText = itemView.findViewById(R.id.server_text);
        }
    }
}
