/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.adapters;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import com.bumptech.glide.Glide;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.domain.PortfolioModel;
import org.chromium.chrome.browser.app.helpers.ImageLoader;
import org.chromium.chrome.browser.crypto_wallet.listeners.OnWalletListItemClick;
import org.chromium.chrome.browser.crypto_wallet.model.WalletListItemModel;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.crypto_wallet.util.WalletConstants;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class WalletNftAdapter extends RecyclerView.Adapter<WalletNftAdapter.ViewHolder> {
    private Context context;
    private List<WalletListItemModel> walletListItemModelList;
    private OnWalletListItemClick onWalletListItemClick;
    private ExecutorService mExecutor;
    private Handler mHandler;

    public WalletNftAdapter() {
        mExecutor = Executors.newSingleThreadExecutor();
        mHandler = new Handler(Looper.getMainLooper());
        walletListItemModelList = new ArrayList<>();
    }

    @Override
    public @NonNull ViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        context = parent.getContext();
        LayoutInflater inflater = LayoutInflater.from(context);
        View walletCoinView = inflater.inflate(R.layout.wallet_nft_item, parent, false);
        return new ViewHolder(walletCoinView);
    }

    @Override
    public void onBindViewHolder(@NonNull ViewHolder holder, int position) {
        WalletListItemModel walletListItemModel = walletListItemModelList.get(position);
        // When a ViewHolder is reused, its listeners may be triggered when modifying the checkbox,
        // potentially leading to unintended modifications of the model.
        holder.resetListeners();

        holder.titleText.setText(walletListItemModel.getTitle());

        PortfolioModel.NftDataModel nftDataModel = walletListItemModel.getNftDataModel();
        holder.subTitleText.setText(nftDataModel.token.symbol);

        holder.itemView.setOnClickListener(v -> {
            onWalletListItemClick.onAssetClick(walletListItemModel.getBlockchainToken());
        });

        Utils.setBitmapResource(mExecutor, mHandler, context, walletListItemModel.getNetworkIcon(),
                walletListItemModel.getIcon(), holder.networkIconImage, null, true);

        if (walletListItemModel.getBlockchainToken() == null
                || !walletListItemModel.getBlockchainToken().logo.isEmpty()) {
            Utils.setBitmapResource(mExecutor, mHandler, context, walletListItemModel.getIconPath(),
                    walletListItemModel.getIcon(), holder.iconImg, null, true);
        } else {
            if (walletListItemModel.hasNftImageLink()
                    && ImageLoader.isSupported(nftDataModel.nftMetadata.mImageUrl)) {
                String url = nftDataModel.nftMetadata.mImageUrl;
                ImageLoader.downloadImage(url, Glide.with(context), false,
                        WalletConstants.RECT_SHARP_ROUNDED_CORNERS_DP, holder.iconImg, null);
            } else {
                Utils.setBlockiesBitmapCustomAsset(mExecutor, mHandler, holder.iconImg,
                        walletListItemModel.getBlockchainToken().contractAddress,
                        walletListItemModel.getBlockchainToken().symbol,
                        context.getResources().getDisplayMetrics().density, null, context, false,
                        (float) 0.9, false);
            }
        }
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

    public void removeItem(WalletListItemModel item) {
        int index = walletListItemModelList.indexOf(item);
        if (index != -1) {
            walletListItemModelList.remove(index);
            notifyItemRemoved(index);
        }
    }

    public void addItem(WalletListItemModel item) {
        walletListItemModelList.add(item);
        notifyItemInserted(walletListItemModelList.size() - 1);
    }

    public void clear() {
        int size = walletListItemModelList.size();
        walletListItemModelList.clear();
        notifyItemRangeRemoved(0, size);
    }

    static class ViewHolder extends RecyclerView.ViewHolder {
        public final ImageView iconImg;
        public final ImageView networkIconImage;
        public final TextView titleText;
        public final TextView subTitleText;

        public ViewHolder(View itemView) {
            super(itemView);
            iconImg = itemView.findViewById(R.id.icon);
            networkIconImage = itemView.findViewById(R.id.network_icon);
            titleText = itemView.findViewById(R.id.title);
            subTitleText = itemView.findViewById(R.id.sub_title);
        }

        public void resetListeners() {
            itemView.setOnClickListener(null);
        }

        public void disableClick() {
            resetListeners();
            itemView.setClickable(false);
        }
    }
}
