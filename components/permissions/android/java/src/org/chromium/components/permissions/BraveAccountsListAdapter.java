/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.components.permissions;

import android.content.Context;
import android.graphics.Bitmap;
import android.os.Handler;
import android.os.Looper;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.chrome.browser.crypto_wallet.util.Blockies;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class BraveAccountsListAdapter
        extends RecyclerView.Adapter<BraveAccountsListAdapter.ViewHolder> {
    private Context mContext;
    private AccountInfo[] mAccountInfo;
    private ExecutorService mExecutor;
    private Handler mHandler;
    private List<Integer> mCheckedPositions = new ArrayList<>();

    public BraveAccountsListAdapter(AccountInfo[] accountInfo) {
        assert accountInfo != null;
        mAccountInfo = accountInfo;
        mExecutor = Executors.newSingleThreadExecutor();
        mHandler = new Handler(Looper.getMainLooper());
    }

    @Override
    public @NonNull BraveAccountsListAdapter.ViewHolder onCreateViewHolder(
            ViewGroup parent, int viewType) {
        mContext = parent.getContext();
        LayoutInflater inflater = LayoutInflater.from(mContext);
        View view = inflater.inflate(R.layout.brave_wallet_accounts_list_item, parent, false);
        return new ViewHolder(view);
    }

    @Override
    public void onBindViewHolder(
            @NonNull BraveAccountsListAdapter.ViewHolder holder, int position) {
        final int arrayPosition = position;
        holder.titleText.setText(mAccountInfo[arrayPosition].name);
        holder.subTitleText.setText(stripAccountAddress(mAccountInfo[arrayPosition].address));
        setBlockiesBitmapResource(holder.iconImg, mAccountInfo[arrayPosition].address);
        holder.accountCheck.setOnCheckedChangeListener(
                new CompoundButton.OnCheckedChangeListener() {
                    @Override
                    public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                        if (isChecked) {
                            mCheckedPositions.add((Integer) arrayPosition);
                        } else {
                            mCheckedPositions.remove((Integer) arrayPosition);
                        }
                    }
                });
    }

    @Override
    public int getItemCount() {
        return mAccountInfo.length;
    }

    public AccountInfo[] getCheckedAccounts() {
        AccountInfo[] checkedAccounts = new AccountInfo[mCheckedPositions.size()];
        for (int i = 0; i < mCheckedPositions.size(); i++) {
            checkedAccounts[i] = mAccountInfo[mCheckedPositions.get(i)];
        }

        return checkedAccounts;
    }

    public static class ViewHolder extends RecyclerView.ViewHolder {
        public ImageView iconImg;
        public TextView titleText;
        public TextView subTitleText;
        public CheckBox accountCheck;

        public ViewHolder(View itemView) {
            super(itemView);
            this.iconImg = itemView.findViewById(R.id.icon);
            this.titleText = itemView.findViewById(R.id.title);
            this.subTitleText = itemView.findViewById(R.id.sub_title);
            this.accountCheck = itemView.findViewById(R.id.accountCheck);
        }
    }

    private void setBlockiesBitmapResource(ImageView iconImg, String source) {
        mExecutor.execute(() -> {
            final Bitmap bitmap = Blockies.createIcon(source, true);
            mHandler.post(() -> {
                if (iconImg != null) {
                    iconImg.setImageBitmap(bitmap);
                }
            });
        });
    }

    private String stripAccountAddress(String address) {
        String newAddress = "";

        if (address.length() > 6) {
            newAddress = address.substring(0, 6) + "***" + address.substring(address.length() - 5);
        }

        return newAddress;
    }
}
