/* Copyright (c) 2019 The Brave Authors. All rights reserved.
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

import android.util.Pair;
import android.content.Context;
import org.chromium.base.ContextUtils;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.ItemTouchHelper;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.chrome.R;

import java.util.List;

public class CurrencyListAdapter extends RecyclerView.Adapter<RecyclerView.ViewHolder> {
    private Context mContext = ContextUtils.getApplicationContext();
    private List<CoinNetworkModel> currencyList;

    @NonNull
    @Override
    public RecyclerView.ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
        View view = LayoutInflater.from(parent.getContext())
                    .inflate(R.layout.binance_currency_item, parent, false);
        return new CurrencyViewHolder(view);
    }

    @SuppressLint("ClickableViewAccessibility")
    @Override
    public void onBindViewHolder(@NonNull final RecyclerView.ViewHolder holder, int position) {
        CurrencyViewHolder currencyViewHolder = (CurrencyViewHolder) holder;
        CoinNetworkModel coinNetworkModel = currencyList.get(position);
        currencyViewHolder.currencyText.setText(coinNetworkModel.getCoin());
        currencyViewHolder.currencyImageView.setImageResource(coinNetworkModel.getCoinRes());
        BinanceAccountBalance binanceAccountBalance = BinanceWidgetManager.getInstance().getBinanceAccountBalance();
        if (binanceAccountBalance.getCurrencyValue(coinNetworkModel.getCoin()) != null) {
            Pair<Double, Double> currencyValue = binanceAccountBalance.getCurrencyValue(coinNetworkModel.getCoin());
            currencyViewHolder.currencyValueText.setText(String.valueOf(currencyValue.first));
            currencyViewHolder.currencyUsdText.setText(String.format(mContext.getResources().getString(R.string.usd_balance), String.valueOf(currencyValue.second)));
        }
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
        TextView currencyValueText;
        TextView currencyUsdText;

        CurrencyViewHolder(View itemView) {
            super(itemView);

            currencyImageView = itemView.findViewById(R.id.currency_image);
            currencyText = itemView.findViewById(R.id.currency_text);
            currencyValueText = itemView.findViewById(R.id.currency_value_text);
            currencyUsdText = itemView.findViewById(R.id.currency_usd_text);
        }
    }
}