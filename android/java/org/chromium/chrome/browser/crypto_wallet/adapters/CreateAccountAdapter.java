/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.adapters;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.model.CryptoAccountTypeInfo;

import java.util.List;

public class CreateAccountAdapter extends RecyclerView.Adapter<CreateAccountAdapter.ViewHolder> {
    private final LayoutInflater inflater;
    private OnCreateAccountClickListener mCreateAccountClickListener;
    private List<CryptoAccountTypeInfo> mCryptoAccountTypeInfos;

    public CreateAccountAdapter(
            Context context, List<CryptoAccountTypeInfo> cryptoAccountTypeInfos) {
        mCryptoAccountTypeInfos = cryptoAccountTypeInfos;
        inflater = LayoutInflater.from(context);
    }

    @Override
    public @NonNull ViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        View view = inflater.inflate(R.layout.item_create_account, parent, false);
        return new ViewHolder(view);
    }

    @Override
    public void onBindViewHolder(@NonNull ViewHolder holder, int position) {
        final CryptoAccountTypeInfo accountTypeInfo = mCryptoAccountTypeInfos.get(position);

        holder.tvTitle.setText(accountTypeInfo.getName());
        holder.tvDesc.setText(accountTypeInfo.getDesc());
        holder.ivNetworkPicture.setImageResource(accountTypeInfo.getIcon());

        holder.itemView.setOnClickListener(
                v -> {
                    assert mCreateAccountClickListener != null;
                    mCreateAccountClickListener.onAccountClick(accountTypeInfo);
                });
    }

    @Override
    public int getItemCount() {
        return mCryptoAccountTypeInfos.size();
    }

    public void setOnAccountItemSelected(OnCreateAccountClickListener listener) {
        mCreateAccountClickListener = listener;
    }

    static class ViewHolder extends RecyclerView.ViewHolder {
        ImageView ivNetworkPicture;
        TextView tvTitle;
        TextView tvDesc;

        ViewHolder(View itemView) {
            super(itemView);
            ivNetworkPicture = itemView.findViewById(R.id.item_create_account_iv_icon);
            tvTitle = itemView.findViewById(R.id.item_create_account_tv_title);
            tvDesc = itemView.findViewById(R.id.item_create_account_tv_desc);
        }
    }

    public interface OnCreateAccountClickListener {
        void onAccountClick(CryptoAccountTypeInfo cryptoAccountTypeInfo);
    }
}
