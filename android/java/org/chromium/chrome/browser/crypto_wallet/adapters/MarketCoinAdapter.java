/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.adapters;

import static org.chromium.ui.base.ViewUtils.dpToPx;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import com.bumptech.glide.Glide;

import org.chromium.brave_wallet.mojom.CoinMarket;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.domain.MarketModel;
import org.chromium.chrome.browser.app.helpers.ImageLoader;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;

import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

public class MarketCoinAdapter extends RecyclerView.Adapter<MarketCoinAdapter.ViewHolder> {
    private static final int PERCENTAGE_CHANGE_DRAWABLE_PADDING_PX = 8;
    private final MarketModel mMarketModel;
    private final List<CoinMarket> mCoinMarkets;

    private final Context mContext;

    public MarketCoinAdapter(
            @NonNull final Context context, @NonNull final MarketModel marketModel) {
        mContext = context;
        mMarketModel = marketModel;
        mCoinMarkets = new ArrayList<>();
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
        holder.itemView.setOnClickListener(
                view -> { Utils.openAssetDetailsActivity(mContext, coinMarket); });
        holder.coinName.setText(coinMarket.name);
        holder.symbol.setText(coinMarket.symbol.toUpperCase(Locale.ENGLISH));

        holder.price.setText(mMarketModel.getFormattedPrice(coinMarket.currentPrice));
        setPriceIndicator(holder.changePercentage, coinMarket.priceChangePercentage24h);

        ImageLoader.downloadImage(
                coinMarket.image, Glide.with(mContext), true, 0, holder.icon, null);
    }

    @Override
    public int getItemCount() {
        return mCoinMarkets.size();
    }

    public void setCoinMarkets(@NonNull final List<CoinMarket> coinMarkets) {
        final int itemCount = mCoinMarkets.size();
        mCoinMarkets.clear();
        mCoinMarkets.addAll(coinMarkets);
        notifyItemRangeRemoved(0, itemCount);
        notifyItemRangeInserted(0, coinMarkets.size());
    }

    private void setPriceIndicator(TextView textView, double percentageChange) {
        if (percentageChange > 0) {
            textView.setTextColor(mContext.getColor(R.color.brave_wallet_day_night_bull));
            textView.setCompoundDrawablesRelativeWithIntrinsicBounds(
                    R.drawable.ic_bull_arrow, 0, 0, 0);
        } else {
            textView.setTextColor(mContext.getColor(R.color.brave_wallet_day_night_bear));
            textView.setCompoundDrawablesRelativeWithIntrinsicBounds(
                    R.drawable.ic_bear_arrow, 0, 0, 0);
        }

        textView.setText(mMarketModel.getFormattedPercentageChange(percentageChange));
        textView.setCompoundDrawablePadding(dpToPx(mContext.getResources().getDisplayMetrics(),
                PERCENTAGE_CHANGE_DRAWABLE_PADDING_PX));
    }

    static class ViewHolder extends RecyclerView.ViewHolder {
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
