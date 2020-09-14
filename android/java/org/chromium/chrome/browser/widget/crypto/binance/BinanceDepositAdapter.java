/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
package org.chromium.chrome.browser.widget.crypto.binance;

import android.annotation.SuppressLint;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;
import android.text.TextUtils;

import android.util.Pair;
import android.content.Context;
import org.chromium.base.ContextUtils;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.ItemTouchHelper;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.widget.crypto.binance.CoinNetworkModel;

import java.util.List;

public class BinanceDepositAdapter extends RecyclerView.Adapter<RecyclerView.ViewHolder> {
    private Context mContext = ContextUtils.getApplicationContext();
    private List<CoinNetworkModel> currencyList;
    private BinanceDepositListener binanceDepositListener;

    public interface BinanceDepositListener {
        void onItemClick(CoinNetworkModel coinNetworkModel);
    }

    public void setBinanceDepositListener(BinanceDepositListener binanceDepositListener) {
        this.binanceDepositListener = binanceDepositListener;
    }

    @NonNull
    @Override
    public RecyclerView.ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
        View view = LayoutInflater.from(parent.getContext())
                    .inflate(R.layout.binance_deposit_item, parent, false);
        return new CurrencyViewHolder(view);
    }

    @SuppressLint("ClickableViewAccessibility")
    @Override
    public void onBindViewHolder(@NonNull final RecyclerView.ViewHolder holder, int position) {
        CurrencyViewHolder currencyViewHolder = (CurrencyViewHolder) holder;
        CoinNetworkModel coinNetworkModel = currencyList.get(position);
        currencyViewHolder.currencyText.setText(coinNetworkModel.getCoin() + (TextUtils.isEmpty(coinNetworkModel.getCoinDesc()) ? "" : " (" + coinNetworkModel.getCoinDesc() + ")"));
        currencyViewHolder.currencyImageView.setImageResource(coinNetworkModel.getCoinRes());
        currencyViewHolder.rootView.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (binanceDepositListener != null) {
                    binanceDepositListener.onItemClick(coinNetworkModel);
                }
            }
        });
    }

    @Override
    public int getItemCount() {
        return currencyList == null ? 0 : currencyList.size();
    }

    void setCurrencyList(List<CoinNetworkModel> currencyList) {
        this.currencyList = currencyList;
        notifyDataSetChanged();
    }

    private class CurrencyViewHolder extends RecyclerView.ViewHolder {
        ImageView currencyImageView;
        TextView currencyText;
        View rootView;

        CurrencyViewHolder(View itemView) {
            super(itemView);

            rootView = itemView;
            currencyImageView = itemView.findViewById(R.id.currency_image);
            currencyText = itemView.findViewById(R.id.currency_text);
        }
    }
}