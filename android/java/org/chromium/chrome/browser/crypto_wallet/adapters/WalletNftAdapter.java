/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.adapters;

import android.annotation.SuppressLint;
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

import org.chromium.brave_wallet.mojom.BlockchainToken;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.domain.PortfolioModel;
import org.chromium.chrome.browser.app.helpers.ImageLoader;
import org.chromium.chrome.browser.crypto_wallet.listeners.OnWalletListItemClick;
import org.chromium.chrome.browser.crypto_wallet.model.WalletListItemModel;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class WalletNftAdapter extends RecyclerView.Adapter<WalletNftAdapter.ViewHolder> {
    private Context context;
    private List<WalletListItemModel> walletListItemModelList = new ArrayList<>();
    private OnWalletListItemClick onWalletListItemClick;
    private ExecutorService mExecutor;
    private Handler mHandler;
    private int previousSelectedPos;

    public WalletNftAdapter() {
        mExecutor = Executors.newSingleThreadExecutor();
        mHandler = new Handler(Looper.getMainLooper());
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
        // When ViewHolder is re-used, it has the obeservers which are fired when
        // we modifying checkbox. This may cause unwanted modifying of the model
        holder.resetObservers();

        holder.titleText.setText(walletListItemModel.getTitle());

        PortfolioModel.NftDataModel nftDataModel = walletListItemModel.getNftDataModel();
        holder.subTitleText.setText(nftDataModel.token.name);

        holder.itemView.setOnClickListener(v -> {
            onWalletListItemClick.onAssetClick(walletListItemModel.getBlockchainToken());
        });

        if (walletListItemModel.getBlockchainToken() == null
                || !walletListItemModel.getBlockchainToken().logo.isEmpty()) {
            Utils.setBitmapResource(mExecutor, mHandler, context, walletListItemModel.getIconPath(),
                    walletListItemModel.getIcon(), holder.iconImg, null, true);
        } else {
            if (walletListItemModel.hasNftImageLink()
                    && ImageLoader.isSupported(nftDataModel.nftMetadata.mImageUrl)) {
                String url = nftDataModel.nftMetadata.mImageUrl;
                ImageLoader.downloadImage(url, context, false, holder.iconImg, null);
            } else {
                Utils.setBlockiesBitmapCustomAsset(mExecutor, mHandler, holder.iconImg,
                        walletListItemModel.getBlockchainToken().contractAddress,
                        walletListItemModel.getBlockchainToken().symbol,
                        context.getResources().getDisplayMetrics().density, null, context, false,
                        (float) 0.9);
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

    /**
     * Update the list's selection icon to the passed account
     * @param title is the account name
     * @param subTitle is the account address
     */
    public void updateSelectedNetwork(String title, String subTitle) {
        for (int i = 0; i < walletListItemModelList.size(); i++) {
            WalletListItemModel walletListItemModel = walletListItemModelList.get(i);
            if (walletListItemModel.getTitle().equals(title)
                    && walletListItemModel.getSubTitle().equals(subTitle)) {
                updateSelectedNetwork(i);
                break;
            }
        }
    }

    private void updateSelectedNetwork(int selectedAccountPosition) {
        walletListItemModelList.get(previousSelectedPos).setIsUserSelected(false);
        notifyItemChanged(previousSelectedPos);

        walletListItemModelList.get(selectedAccountPosition).setIsUserSelected(true);
        previousSelectedPos = selectedAccountPosition;
        notifyItemChanged(selectedAccountPosition);
    }

    @SuppressLint("NotifyDataSetChanged")
    public void removeItem(WalletListItemModel item) {
        walletListItemModelList.remove(item);
        notifyDataSetChanged();
    }

    @SuppressLint("NotifyDataSetChanged")
    public void addItem(WalletListItemModel item) {
        walletListItemModelList.add(item);
        notifyDataSetChanged();
    }

    @SuppressLint("NotifyDataSetChanged")
    public void clear() {
        walletListItemModelList.clear();
        notifyDataSetChanged();
    }

    public static class ViewHolder extends RecyclerView.ViewHolder {
        public ImageView iconImg;
        public TextView titleText;
        public TextView subTitleText;

        public ViewHolder(View itemView) {
            super(itemView);
            this.iconImg = itemView.findViewById(R.id.icon);
            this.titleText = itemView.findViewById(R.id.title);
            this.subTitleText = itemView.findViewById(R.id.sub_title);
        }

        public void resetObservers() {
            itemView.setOnClickListener(null);
        }

        public void disableClick() {
            resetObservers();
            itemView.setClickable(false);
        }
    }
}
