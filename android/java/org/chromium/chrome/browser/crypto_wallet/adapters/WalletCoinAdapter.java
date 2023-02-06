/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.adapters;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.Intent;
import android.graphics.drawable.BitmapDrawable;
import android.os.Handler;
import android.os.Looper;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.brave_wallet.mojom.TransactionStatus;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.domain.PortfolioModel;
import org.chromium.chrome.browser.crypto_wallet.listeners.OnWalletListItemClick;
import org.chromium.chrome.browser.crypto_wallet.model.WalletListItemModel;
import org.chromium.chrome.browser.crypto_wallet.util.ImageLoader;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;

import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class WalletCoinAdapter extends RecyclerView.Adapter<WalletCoinAdapter.ViewHolder> {
    public enum AdapterType {
        VISIBLE_ASSETS_LIST,
        EDIT_VISIBLE_ASSETS_LIST,
        ACCOUNTS_LIST,
        SELECT_ACCOUNTS_LIST,
        BUY_ASSETS_LIST,
        SEND_ASSETS_LIST,
        SWAP_TO_ASSETS_LIST,
        SWAP_FROM_ASSETS_LIST;
    }

    private Context context;
    private List<WalletListItemModel> walletListItemModelList = new ArrayList<>();
    private List<WalletListItemModel> walletListItemModelListCopy = new ArrayList<>();
    private List<Integer> mCheckedPositions = new ArrayList<>();
    private OnWalletListItemClick onWalletListItemClick;
    private int walletListItemType;
    private AdapterType mType;
    private ExecutorService mExecutor;
    private Handler mHandler;
    private int previousSelectedPos;

    public WalletCoinAdapter(AdapterType type) {
        mType = type;
        mExecutor = Executors.newSingleThreadExecutor();
        mHandler = new Handler(Looper.getMainLooper());
    }

    @Override
    public @NonNull ViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        context = parent.getContext();
        LayoutInflater inflater = LayoutInflater.from(context);
        View walletCoinView = inflater.inflate(R.layout.wallet_coin_list_item, parent, false);
        return new ViewHolder(walletCoinView);
    }

    @Override
    public void onBindViewHolder(@NonNull ViewHolder holder, int position) {
        WalletListItemModel walletListItemModel = walletListItemModelList.get(position);
        // When ViewHolder is re-used, it has the obeservers which are fired when
        // we modifying checkbox. This may cause unwanted modifying of the model
        holder.resetObservers();

        holder.titleText.setText(walletListItemModel.getTitle());
        holder.subTitleText.setText(
                mType == AdapterType.ACCOUNTS_LIST || mType == AdapterType.SELECT_ACCOUNTS_LIST
                        ? Utils.stripAccountAddress(walletListItemModel.getSubTitle())
                        : walletListItemModel.getSubTitle());
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
                onWalletListItemClick.onTransactionClick(walletListItemModel.getTransactionInfo());
            } else if (walletListItemType == Utils.ASSET_ITEM) {
                if (mType == AdapterType.BUY_ASSETS_LIST || mType == AdapterType.SEND_ASSETS_LIST
                        || mType == AdapterType.SWAP_TO_ASSETS_LIST
                        || mType == AdapterType.SWAP_FROM_ASSETS_LIST) {
                    for (int i = 0; i < walletListItemModelListCopy.size(); i++) {
                        WalletListItemModel item = walletListItemModelListCopy.get(i);
                        if (item.getTitle().equals(holder.titleText.getText())
                                || item.getSubTitle().equals(holder.subTitleText.getText())) {
                            mCheckedPositions.add((Integer) i);
                            break;
                        }
                    }
                }
                if (mType != AdapterType.EDIT_VISIBLE_ASSETS_LIST) {
                    onWalletListItemClick.onAssetClick(walletListItemModel.getBlockchainToken());
                }
            } else {
                onWalletListItemClick.onAccountClick(walletListItemModel);
                if (mType == AdapterType.SELECT_ACCOUNTS_LIST) {
                    updateSelectedNetwork(holder.getLayoutPosition());
                }
            }
        });

        if (walletListItemType == Utils.TRANSACTION_ITEM) {
            holder.txStatus.setVisibility(View.VISIBLE);
            holder.txStatus.setText(walletListItemModel.getTxStatus());
            holder.txStatus.setCompoundDrawablesRelativeWithIntrinsicBounds(
                    new BitmapDrawable(
                            context.getResources(), walletListItemModel.getTxStatusBitmap()),
                    null, null, null);
            holder.feeText.setVisibility(View.VISIBLE);
            holder.feeText.setText(String.format(
                    context.getResources().getString(R.string.wallet_tx_fee),
                    String.format(Locale.getDefault(), "%.6f", walletListItemModel.getTotalGas()),
                    walletListItemModel.getChainSymbol(),
                    String.format(
                            Locale.getDefault(), "%.2f", walletListItemModel.getTotalGasFiat())));
            Utils.overlayBitmaps(mExecutor, mHandler, walletListItemModel.getAddressesForBitmap(),
                    holder.iconImg);
            if (walletListItemModel.getTransactionInfo().txStatus == TransactionStatus.REJECTED) {
                holder.disableClick();
            } else {
                holder.itemView.setClickable(true);
            }
        }

        if (mType == AdapterType.EDIT_VISIBLE_ASSETS_LIST || mType == AdapterType.BUY_ASSETS_LIST
                || mType == AdapterType.SEND_ASSETS_LIST || mType == AdapterType.SWAP_TO_ASSETS_LIST
                || mType == AdapterType.SWAP_FROM_ASSETS_LIST) {
            holder.text1Text.setVisibility(View.GONE);
            holder.text2Text.setVisibility(View.GONE);
            if (mType == AdapterType.EDIT_VISIBLE_ASSETS_LIST) {
                holder.assetCheck.setVisibility(View.VISIBLE);
                holder.assetCheck.setChecked(walletListItemModel.getIsUserSelected());
                holder.assetCheck.setOnCheckedChangeListener(
                        new CompoundButton.OnCheckedChangeListener() {
                            @Override
                            public void onCheckedChanged(
                                    CompoundButton buttonView, boolean isChecked) {
                                for (int i = 0; i < walletListItemModelListCopy.size(); i++) {
                                    WalletListItemModel item = walletListItemModelListCopy.get(i);
                                    if (item.getTitle().equals(holder.titleText.getText())
                                            || item.getSubTitle().equals(
                                                    holder.subTitleText.getText())) {
                                        if (isChecked) {
                                            if (!mCheckedPositions.contains((Integer) i)) {
                                                mCheckedPositions.add((Integer) i);
                                            }
                                        } else {
                                            mCheckedPositions.remove((Integer) i);
                                        }
                                        break;
                                    }
                                }
                                if ("noOnClickListener".equals(
                                            (String) holder.assetCheck.getTag())) {
                                    holder.assetCheck.setTag(null);
                                } else {
                                    onWalletListItemClick.onAssetCheckedChanged(
                                            walletListItemModel, holder.assetCheck, isChecked);
                                }
                            }
                        });
            }
        }

        if (mType != AdapterType.ACCOUNTS_LIST && mType != AdapterType.SELECT_ACCOUNTS_LIST) {
            if (walletListItemModel.getBlockchainToken() == null
                    || !walletListItemModel.getBlockchainToken().logo.isEmpty()) {
                Utils.setBitmapResource(mExecutor, mHandler, context,
                        walletListItemModel.getIconPath(), walletListItemModel.getIcon(),
                        holder.iconImg, null, true);
            } else {
                PortfolioModel.NftDataModel nftDataModel = walletListItemModel.getNftDataModel();
                if (walletListItemModel.hasNftImageLink()
                        && ImageLoader.isSupported(nftDataModel.nftMetadata.mImageUrl)) {
                    String url = nftDataModel.nftMetadata.mImageUrl;
                    ImageLoader.createLoadNftRequest(url, context, false).into(holder.iconImg);
                } else {
                    Utils.setBlockiesBitmapCustomAsset(mExecutor, mHandler, holder.iconImg,
                            walletListItemModel.getBlockchainToken().contractAddress,
                            walletListItemModel.getBlockchainToken().symbol,
                            context.getResources().getDisplayMetrics().density, null, context,
                            false, (float) 0.9);
                }
                if (mType == AdapterType.EDIT_VISIBLE_ASSETS_LIST) {
                    onWalletListItemClick.onMaybeShowTrashButton(
                            walletListItemModel, holder.iconTrash);
                    holder.iconTrash.setOnClickListener(
                            v -> { onWalletListItemClick.onTrashIconClick(walletListItemModel); });
                }
            }
        } else if (mType == AdapterType.ACCOUNTS_LIST
                || mType == AdapterType.SELECT_ACCOUNTS_LIST) {
            holder.iconImg.setImageResource(android.R.color.transparent);
            Utils.setBlockiesBitmapResource(
                    mExecutor, mHandler, holder.iconImg, walletListItemModel.getSubTitle(), true);
            holder.itemView.setOnLongClickListener(v -> {
                Utils.saveTextToClipboard(context, walletListItemModel.getSubTitle(),
                        R.string.address_has_been_copied, false);

                return true;
            });
            if (mType == AdapterType.SELECT_ACCOUNTS_LIST) {
                holder.ivSelected.setVisibility(
                        walletListItemModel.getIsUserSelected() ? View.VISIBLE : View.INVISIBLE);
            }
        }
    }

    @Override
    public int getItemCount() {
        return walletListItemModelList.size();
    }

    public void setWalletCoinAdapterType(AdapterType type) {
        mType = type;
    }

    public void setWalletListItemModelList(List<WalletListItemModel> walletListItemModelList) {
        this.walletListItemModelList = walletListItemModelList;
        if (mType == AdapterType.EDIT_VISIBLE_ASSETS_LIST || mType == AdapterType.BUY_ASSETS_LIST
                || mType == AdapterType.SEND_ASSETS_LIST || mType == AdapterType.SWAP_TO_ASSETS_LIST
                || mType == AdapterType.SWAP_FROM_ASSETS_LIST) {
            walletListItemModelListCopy.addAll(walletListItemModelList);
            mCheckedPositions.clear();
        }
        for (int i = 0; i < walletListItemModelListCopy.size(); i++) {
            if (walletListItemModelListCopy.get(i).getIsUserSelected()) {
                mCheckedPositions.add((Integer) i);
            }
        }
    }

    public List<WalletListItemModel> getCheckedAssets() {
        List<WalletListItemModel> checkedAssets = new ArrayList<>();
        for (Integer position : mCheckedPositions) {
            if (position >= walletListItemModelListCopy.size()) {
                assert false;
                continue;
            }
            checkedAssets.add(walletListItemModelListCopy.get(position));
        }

        return checkedAssets;
    }

    public void setOnWalletListItemClick(OnWalletListItemClick onWalletListItemClick) {
        this.onWalletListItemClick = onWalletListItemClick;
    }

    public void setWalletListItemType(int walletListItemType) {
        this.walletListItemType = walletListItemType;
    }

    public void onTransactionUpdate(TransactionInfo txInfo) {
        for (int i = 0; i < walletListItemModelList.size(); i++) {
            WalletListItemModel item = walletListItemModelList.get(i);
            TransactionInfo transactionInfo = item.getTransactionInfo();
            if (transactionInfo != null && txInfo.id.equals(transactionInfo.id)) {
                Utils.updateWalletCoinTransactionItem(item, txInfo, context);
                walletListItemModelList.set(i, item);
                notifyItemChanged(i);
                break;
            }
        }
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

    public static class ViewHolder extends RecyclerView.ViewHolder {
        public ImageView iconImg;
        public TextView titleText;
        public TextView subTitleText;
        public TextView txStatus;
        public TextView text1Text;
        public TextView text2Text;
        public CheckBox assetCheck;
        public LinearLayout feeLayout;
        public TextView feeText;
        public ImageView iconTrash;
        public ImageView ivSelected;

        public ViewHolder(View itemView) {
            super(itemView);
            this.iconImg = itemView.findViewById(R.id.icon);
            this.titleText = itemView.findViewById(R.id.title);
            this.txStatus = itemView.findViewById(R.id.status);
            this.subTitleText = itemView.findViewById(R.id.sub_title);
            this.text1Text = itemView.findViewById(R.id.text1);
            this.text2Text = itemView.findViewById(R.id.text2);
            this.assetCheck = itemView.findViewById(R.id.assetCheck);
            this.feeText = itemView.findViewById(R.id.fee_text);
            this.iconTrash = itemView.findViewById(R.id.icon_trash);
            this.ivSelected = itemView.findViewById(R.id.iv_selected);
        }

        public void resetObservers() {
            itemView.setOnClickListener(null);
            assetCheck.setOnCheckedChangeListener(null);
        }

        public void disableClick() {
            resetObservers();
            itemView.setClickable(false);
        }
    }

    @SuppressLint("NotifyDataSetChanged")
    public void filter(String text) {
        walletListItemModelList.clear();
        if (text.isEmpty()) {
            walletListItemModelList.addAll(walletListItemModelListCopy);
        } else {
            text = text.toLowerCase(Locale.getDefault());
            for (WalletListItemModel item : walletListItemModelListCopy) {
                if (item.getTitle().toLowerCase(Locale.getDefault()).contains(text)
                        || item.getSubTitle().toLowerCase(Locale.getDefault()).contains(text)) {
                    walletListItemModelList.add(item);
                }
            }
        }
        notifyDataSetChanged();
    }

    @SuppressLint("NotifyDataSetChanged")
    public void removeItem(WalletListItemModel item) {
        walletListItemModelList.remove(item);
        walletListItemModelListCopy.remove(item);
        notifyDataSetChanged();
    }

    @SuppressLint("NotifyDataSetChanged")
    public void addItem(WalletListItemModel item) {
        walletListItemModelList.add(item);
        walletListItemModelListCopy.add(item);
        notifyDataSetChanged();
    }

    @SuppressLint("NotifyDataSetChanged")
    public void clear() {
        walletListItemModelList.clear();
        walletListItemModelListCopy.clear();
        mCheckedPositions.clear();
        notifyDataSetChanged();
    }
}
