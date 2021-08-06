/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.adapters;

import android.content.Context;
import android.content.Intent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.activities.AssetDetailActivity;
import org.chromium.chrome.browser.crypto_wallet.listeners.OnWalletListItemClick;
import org.chromium.chrome.browser.crypto_wallet.model.WalletListItemModel;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;

import java.util.ArrayList;
import java.util.List;

public class WalletCoinAdapter extends RecyclerView.Adapter<WalletCoinAdapter.ViewHolder> {
    private Context context;
    private List<WalletListItemModel> walletListItemModelList = new ArrayList<>();
    private OnWalletListItemClick onWalletListItemClick;
    private int walletListItemType;
    @Override
    public @NonNull WalletCoinAdapter.ViewHolder onCreateViewHolder(
            ViewGroup parent, int viewType) {
        context = parent.getContext();
        LayoutInflater inflater = LayoutInflater.from(context);
        View walletCoinView = inflater.inflate(R.layout.wallet_coin_list_item, parent, false);
        return new ViewHolder(walletCoinView);
    }

    @Override
    public void onBindViewHolder(@NonNull WalletCoinAdapter.ViewHolder holder, int position) {
        WalletListItemModel walletListItemModel = walletListItemModelList.get(position);
        holder.iconImg.setImageResource(walletListItemModel.getIcon());
        holder.titleText.setText(walletListItemModel.getTitle());
        holder.subTitleText.setText(walletListItemModel.getSubTitle());
        if (walletListItemModel.getText1() != null) {
            holder.text1Text.setVisibility(View.VISIBLE);
            holder.text1Text.setText(walletListItemModel.getText1());
        }

        if (walletListItemModel.getText2() != null) {
            holder.text2Text.setVisibility(View.VISIBLE);
            holder.text2Text.setText(walletListItemModel.getText2());
        }

        holder.itemView.setOnClickListener(v -> {
            if (walletListItemType == Utils.TRANSACTION_ITEM) {
                onWalletListItemClick.onTransactionClick();
            } else if (walletListItemType == Utils.ASSET_ITEM) {
                onWalletListItemClick.onAssetClick();
            } else {
                onWalletListItemClick.onAccountClick();
            }
        });
    }

    @Override
    public int getItemCount() {
        return walletListItemModelList.size();
    }

    public void setWalletListItemModelList(List<WalletListItemModel> walletListItemModelList) {
        this.walletListItemModelList = walletListItemModelList;
    }

    public void setOnWalletListItemClick(OnWalletListItemClick onWalletListItemClick) {
        this.onWalletListItemClick = onWalletListItemClick;
    }

    public void setWalletListItemType(int walletListItemType) {
        this.walletListItemType = walletListItemType;
    }

    public static class ViewHolder extends RecyclerView.ViewHolder {
        public ImageView iconImg;
        public TextView titleText;
        public TextView subTitleText;
        public TextView text1Text;
        public TextView text2Text;

        public ViewHolder(View itemView) {
            super(itemView);
            this.iconImg = itemView.findViewById(R.id.icon);
            this.titleText = itemView.findViewById(R.id.title);
            this.subTitleText = itemView.findViewById(R.id.sub_title);
            this.text1Text = itemView.findViewById(R.id.text1);
            this.text2Text = itemView.findViewById(R.id.text2);
        }
    }
}
