/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.adapters;

import static org.chromium.ui.base.ViewUtils.dpToPx;

import org.chromium.brave_wallet.mojom.CoinMarket;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;
import android.os.Handler;
import android.os.Looper;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.chrome.browser.app.helpers.ImageLoader;

import com.bumptech.glide.Glide;

import java.text.DecimalFormatSymbols;
import java.text.DecimalFormat;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.Locale;
import java.math.RoundingMode;

import org.chromium.chrome.R;

public class MarketCoinAdapter extends RecyclerView.Adapter<MarketCoinAdapter.ViewHolder> {
    private static final int PERCENTAGE_CHANGE_DRAWABLE_PADDING_PX = 8;
    private final List<CoinMarket> mCoinMarkets;

    private final Context mContext;
    private final ExecutorService mExecutor;
    private final Handler mHandler;

    private final DecimalFormatSymbols mSymbols;
    private final DecimalFormat mPriceFormatter;
    private final DecimalFormat mPercentageFormatter;

    public MarketCoinAdapter(@NonNull final Context context) {
        mContext = context;
        mExecutor = Executors.newSingleThreadExecutor();
        mHandler = new Handler(Looper.getMainLooper());
        mCoinMarkets = new ArrayList<>();

        mSymbols = new DecimalFormatSymbols(Locale.ENGLISH);
        mPriceFormatter = new DecimalFormat("$0.00##########", mSymbols);
        mPriceFormatter.setGroupingSize(3);
        mPriceFormatter.setGroupingUsed(true);

        mPercentageFormatter = new DecimalFormat("0.00", mSymbols);
        mPercentageFormatter.setRoundingMode(RoundingMode.UP);


    }

    @Override
    public @NonNull ViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        LayoutInflater inflater = LayoutInflater.from(mContext);
        View marketCoinView = inflater.inflate(R.layout.market_coin_list_item, parent, false);
        return new ViewHolder(marketCoinView);
    }

    @Override
    public void onBindViewHolder(@NonNull ViewHolder holder, int position) {
        CoinMarket coinMarket = mCoinMarkets.get(position);
        holder.coinName.setText(coinMarket.name);
        holder.symbol.setText(coinMarket.symbol.toUpperCase(Locale.ENGLISH));

        holder.price.setText(mPriceFormatter.format(coinMarket.currentPrice));
        setPriceIndicator(holder.changePercentage, coinMarket.priceChangePercentage24h);

        ImageLoader.downloadImage(coinMarket.image, Glide.with(mContext), true,
                0, holder.icon, null);
    }

    @Override
    public int getItemCount() {
        return mCoinMarkets.size();
    }

    public void setCoinMarkets(@NonNull final List<CoinMarket> coinMarkets) {
        mCoinMarkets.clear();
        mCoinMarkets.addAll(coinMarkets);
        notifyItemRangeInserted(0, coinMarkets.size());
    }

    private void setPriceIndicator(TextView textView, double percentageChange) {
        if (percentageChange > 0) {
            textView.setTextColor(mContext.getColor(R.color.brave_wallet_day_night_bull));
            textView.setCompoundDrawablesRelativeWithIntrinsicBounds(R.drawable.ic_bull_arrow, 0, 0, 0);
        } else {
            textView.setTextColor(mContext.getColor(R.color.brave_wallet_day_night_bear));
            textView.setCompoundDrawablesRelativeWithIntrinsicBounds(R.drawable.ic_bear_arrow, 0, 0, 0);
        }
        double absoluteChange = Math.abs(percentageChange);
        textView.setText(String.format(Locale.ENGLISH, "%s%%", mPercentageFormatter.format(absoluteChange)));
        textView.setCompoundDrawablePadding(dpToPx(mContext.getResources().getDisplayMetrics(), PERCENTAGE_CHANGE_DRAWABLE_PADDING_PX));
    }

    public static class ViewHolder extends RecyclerView.ViewHolder {
        private final ImageView icon;
        private final TextView coinName;
        private final TextView symbol;
        private final TextView price;
        private final TextView changePercentage;

        public ViewHolder(View itemView) {
            super(itemView);
            icon = itemView.findViewById(R.id.icon);
            coinName = itemView.findViewById(R.id.coin_name);
            symbol = itemView.findViewById(R.id.symbol);
            price = itemView.findViewById(R.id.price);
            changePercentage = itemView.findViewById(R.id.change_percentage);
        }
    }
}