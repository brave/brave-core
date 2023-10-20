/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.site_settings;

import android.annotation.SuppressLint;
import android.content.Context;
import android.util.AttributeSet;

import androidx.annotation.NonNull;
import androidx.preference.Preference;
import androidx.preference.PreferenceViewHolder;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.BraveWalletServiceFactory;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;

public class BraveWalletSolanaConnectedSitesPreference
        extends Preference implements ConnectionErrorHandler,
                                      BraveWalletEthereumConnectedSitesListAdapter
                                              .BraveEthereumPermissionConnectedSitesDelegate {
    private RecyclerView mRecyclerView;
    private BraveWalletService mBraveWalletService;
    private BraveWalletEthereumConnectedSitesListAdapter mAdapter;

    public BraveWalletSolanaConnectedSitesPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    public void onBindViewHolder(@NonNull PreferenceViewHolder holder) {
        super.onBindViewHolder(holder);

        initBraveWalletService();

        mRecyclerView = (RecyclerView) holder.findViewById(R.id.connected_sites_list);
        updateWebSitesList();
    }

    public void destroy() {
        if (mBraveWalletService == null) {
            return;
        }
        mBraveWalletService.close();
        mBraveWalletService = null;
    }

    @SuppressLint("NotifyDataSetChanged")
    private void updateWebSitesList() {
        mBraveWalletService.getWebSitesWithPermission(CoinType.SOL, webSites -> {
            if (mAdapter == null) {
                mAdapter = new BraveWalletEthereumConnectedSitesListAdapter(webSites, this);
                mRecyclerView.setAdapter(mAdapter);
            } else {
                mAdapter.setWebSites(webSites);
                mAdapter.notifyDataSetChanged();
            }
        });
    }

    @Override
    public void removePermission(String webSite) {
        mBraveWalletService.resetWebSitePermission(CoinType.SOL, webSite, success -> {
            if (success) {
                updateWebSitesList();
            }
        });
    }

    @Override
    public void onConnectionError(MojoException e) {
        mBraveWalletService.close();
        mBraveWalletService = null;
        initBraveWalletService();
    }

    private void initBraveWalletService() {
        if (mBraveWalletService != null) {
            return;
        }

        mBraveWalletService = BraveWalletServiceFactory.getInstance().getBraveWalletService(this);
    }
}
