/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.adapters;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.chrome.R;

public class WalletCoinAdapter extends RecyclerView.Adapter<WalletCoinAdapter.ViewHolder> {
    @Override
    public @NonNull WalletCoinAdapter.ViewHolder onCreateViewHolder(
            ViewGroup parent, int viewType) {
        Context context = parent.getContext();
        LayoutInflater inflater = LayoutInflater.from(context);
        View walletCoinView = inflater.inflate(R.layout.wallet_coin_list_item, parent, false);
        return new ViewHolder(walletCoinView);
    }

    @Override
    public void onBindViewHolder(@NonNull WalletCoinAdapter.ViewHolder holder, int position) {
        // TODO add holder view
    }

    @Override
    public int getItemCount() {
        return 10;
    }

    public static class ViewHolder extends RecyclerView.ViewHolder {
        public ViewHolder(View itemView) {
            super(itemView);
        }
    }
}
