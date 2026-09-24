/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.site_settings;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.chrome.R;

public class BraveWalletEthereumConnectedSitesListAdapter
        extends RecyclerView.Adapter<BraveWalletEthereumConnectedSitesListAdapter.ViewHolder> {
    private String[] mWebSites = new String[0];
    private Context mContext;
    private final BraveEthereumPermissionConnectedSitesDelegate mDelegate;

    public interface BraveEthereumPermissionConnectedSitesDelegate {
        default void removePermission(String webSite){};
    }

    public BraveWalletEthereumConnectedSitesListAdapter(
            String[] webSites, BraveEthereumPermissionConnectedSitesDelegate delegate) {
        mWebSites = webSites;
        mDelegate = delegate;
    }

    public void setWebSites(String[] webSites) {
        mWebSites = webSites;
    }

    @Override
    public @NonNull ViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        mContext = parent.getContext();
        LayoutInflater inflater = LayoutInflater.from(mContext);
        View view =
                inflater.inflate(R.layout.brave_wallet_connected_sites_list_item, parent, false);

        return new ViewHolder(view);
    }

    @Override
    public void onBindViewHolder(@NonNull ViewHolder holder, int position) {
        holder.webSite.setText(mWebSites[position]);
        holder.iconTrash.setOnClickListener(v -> {
            if (mDelegate != null) {
                mDelegate.removePermission(mWebSites[position]);
            }
        });
    }

    @Override
    public int getItemCount() {
        return mWebSites.length;
    }

    public static class ViewHolder extends RecyclerView.ViewHolder {
        public TextView webSite;
        public ImageView iconTrash;

        public ViewHolder(View itemView) {
            super(itemView);
            webSite = itemView.findViewById(R.id.web_site);
            iconTrash = itemView.findViewById(R.id.icon_trash);
        }
    }
}
