/* Copyright (c) 2022 The Brave Authors. All rights reserved.
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

import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.domain.NetworkModel;
import org.chromium.chrome.browser.crypto_wallet.util.AndroidUtils;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class NetworkSelectorAdapter
        extends RecyclerView.Adapter<NetworkSelectorAdapter.ViewHolder> {
    private final ExecutorService mExecutor;
    private final Handler mHandler;
    private final LayoutInflater inflater;
    private Context mContext;
    private NetworkClickListener networkClickListener;
    private List<NetworkSelectorItem> mNetworkInfos;
    private String mSelectedNetwork;
    private int mSelectedParentItemPos;

    public NetworkSelectorAdapter(Context context, List<NetworkSelectorItem> networkInfos) {
        mNetworkInfos = networkInfos;
        this.mContext = context;
        inflater = (LayoutInflater.from(context));
        mExecutor = Executors.newSingleThreadExecutor();
        mHandler = new Handler(Looper.getMainLooper());
    }

    @Override
    public int getItemViewType(int position) {
        return mNetworkInfos.get(position).getType().ordinal();
    }

    @Override
    public @NonNull ViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        mContext = parent.getContext();
        View view = inflater.inflate(R.layout.item_network_selector, parent, false);
        return new ViewHolder(view);
    }

    @SuppressLint("NotifyDataSetChanged")
    @Override
    public void onBindViewHolder(@NonNull ViewHolder holder, int position) {
        final NetworkSelectorItem network = mNetworkInfos.get(position);
        holder.tvName.setText(network.getNetworkName());
        AndroidUtils.gone(holder.labelTv);

        switch (network.mType) {
            case SECONDARY:
            case ITEM:
            case TEST:
            case PRIMARY: {
                View.OnClickListener listener = v -> {
                    removePrevSelection();
                    mSelectedParentItemPos = holder.getLayoutPosition();
                    AndroidUtils.show(holder.ivSelected);
                    network.setIsSelected(true);
                    if (networkClickListener != null) {
                        networkClickListener.onNetworkItemSelected(
                                mNetworkInfos.get(mSelectedParentItemPos).mNetworkInfo);
                    }
                };

                if (network.isSelected()) {
                    AndroidUtils.show(holder.ivSelected);
                } else {
                    AndroidUtils.invisible(holder.ivSelected);
                }

                Utils.setBlockiesBitmapResource(mExecutor, mHandler, holder.ivNetworkPicture,
                        network.getNetworkName(), false);
                holder.ivNetworkPicture.setOnClickListener(listener);
                holder.tvName.setOnClickListener(listener);
                break;
            }
            case LABEL: {
                AndroidUtils.gone(holder.ivNetworkPicture, holder.tvName, holder.ivNetworkPicture,
                        holder.ivSelected);
                AndroidUtils.show(holder.labelTv);
                holder.labelTv.setText(network.getNetworkName());
                break;
            }
        }
    }

    @Override
    public int getItemCount() {
        return mNetworkInfos.size();
    }

    @SuppressLint("NotifyDataSetChanged")
    public void addNetworks(NetworkModel.NetworkLists networkLists) {
        // Primary
        mNetworkInfos.add(new NetworkSelectorItem(
                mContext.getString(R.string.brave_wallet_network_filter_primary), "", null,
                Type.LABEL));
        List<NetworkSelectorItem> primaryItems = new ArrayList<>();
        for (NetworkInfo networkInfo : networkLists.mPrimaryNetworkList) {
            primaryItems.add(new NetworkSelectorItem(
                    networkInfo.chainName, networkInfo.symbolName, networkInfo, Type.PRIMARY));
        }
        mNetworkInfos.addAll(primaryItems);

        // Secondary
        mNetworkInfos.add(new NetworkSelectorItem(
                mContext.getString(R.string.brave_wallet_network_filter_secondary), "", null,
                Type.LABEL));
        for (NetworkInfo networkInfo : networkLists.mSecondaryNetworkList) {
            mNetworkInfos.add(new NetworkSelectorItem(
                    networkInfo.chainName, networkInfo.symbolName, networkInfo, Type.SECONDARY));
        }

        // Test
        mNetworkInfos.add(new NetworkSelectorItem(
                mContext.getString(R.string.brave_wallet_network_filter_test), "", null,
                Type.LABEL));
        for (NetworkInfo networkInfo : networkLists.mTestNetworkList) {
            mNetworkInfos.add(new NetworkSelectorItem(
                    networkInfo.chainName, networkInfo.symbolName, networkInfo, Type.TEST));
        }

        if (mSelectedNetwork != null) {
            setSelectedNetwork(mSelectedNetwork);
        }
        notifyDataSetChanged();
    }

    public void setSelectedNetwork(String networkName) {
        mSelectedNetwork = networkName;
        for (int i = 0; i < mNetworkInfos.size(); i++) {
            NetworkSelectorItem networkSelectorItem = mNetworkInfos.get(i);
            if (networkSelectorItem.getNetworkName().equalsIgnoreCase(networkName)) {
                removePrevSelection();
                networkSelectorItem.setIsSelected(true);
                mSelectedParentItemPos = i;
                notifyItemChanged(mSelectedParentItemPos);
                break;
            }
        }
    }

    public void setOnNetworkItemSelected(NetworkClickListener networkClickListener) {
        this.networkClickListener = networkClickListener;
    }

    private void removePrevSelection() {
        if (mNetworkInfos.get(mSelectedParentItemPos).isSelected()) {
            mNetworkInfos.get(mSelectedParentItemPos).setIsSelected(false);
            notifyItemChanged(mSelectedParentItemPos);
        }
    }

    enum Type { ITEM, PRIMARY, SECONDARY, TEST, LABEL }

    public interface NetworkClickListener {
        void onNetworkItemSelected(NetworkInfo networkInfo);
    }

    static class ViewHolder extends RecyclerView.ViewHolder {
        ImageView ivNetworkPicture;
        TextView tvName;
        ImageView ivSelected;
        TextView labelTv;

        ViewHolder(View itemView) {
            super(itemView);
            tvName = itemView.findViewById(R.id.tv_item_network_name);
            ivNetworkPicture = itemView.findViewById(R.id.iv_item_network_picture);
            ivSelected = itemView.findViewById(R.id.iv_item_network_selector_selected);
            labelTv = itemView.findViewById(R.id.iv_item_network_label);
        }
    }

    public static class NetworkSelectorItem {
        private NetworkInfo mNetworkInfo;
        private String mNetworkName;
        private String mNetworkShortName;
        private boolean mIsSelected;
        private Type mType;

        public NetworkSelectorItem(
                String networkName, String networkShortName, NetworkInfo networkInfo) {
            this.mNetworkName = networkName;
            this.mNetworkShortName = networkShortName;
            this.mNetworkInfo = networkInfo;
        }

        public NetworkSelectorItem(
                String networkName, String networkShortName, NetworkInfo networkInfo, Type type) {
            this(networkName, networkShortName, networkInfo);
            this.mType = type;
            if (mType == null) {
                mType = Type.ITEM;
            }
        }

        public Type getType() {
            return mType;
        }

        public String getNetworkName() {
            return mNetworkName;
        }

        public void setNetworkName(String mNetworkName) {
            this.mNetworkName = mNetworkName;
        }

        public String getNetworkShortName() {
            return mNetworkShortName;
        }

        public void setNetworkShortName(String mNetworkShortName) {
            this.mNetworkShortName = mNetworkShortName;
        }

        public boolean isSelected() {
            return mIsSelected;
        }

        public void setIsSelected(boolean mIsSelected) {
            this.mIsSelected = mIsSelected;
        }
    }
}
