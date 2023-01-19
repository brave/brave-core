/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.adapters;

import android.annotation.SuppressLint;
import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.util.Pair;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.presenters.NetworkInfoPresenter;
import org.chromium.chrome.browser.crypto_wallet.util.AndroidUtils;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.mojo.bindings.Callbacks;

import java.util.ArrayList;
import java.util.Collections;
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
    private Pair<Integer, Integer> mSelectedNestedItemPos;
    private final Callbacks.Callback2<Integer, Integer> nestedCallback = (parentPos, childPos) -> {
        removePrevParentSelection();
        // Nested items are mark as selected first and then the position of nested item is retrieved
        // So do not remove the selection from the same click position
        Pair<Integer, Integer> pair = Pair.create(parentPos, childPos);
        if (!mSelectedNestedItemPos.equals(pair)) {
            removePrevNestedSelection();
        }
        mSelectedNestedItemPos = pair;
    };

    public NetworkSelectorAdapter(Context context, List<NetworkSelectorItem> networkInfos) {
        mNetworkInfos = networkInfos;
        this.mContext = context;
        inflater = (LayoutInflater.from(context));
        mExecutor = Executors.newSingleThreadExecutor();
        mHandler = new Handler(Looper.getMainLooper());
        mSelectedNestedItemPos = Pair.create(0, 0);
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
            case PRIMARY: {
                View.OnClickListener listener = v -> {
                    removePrevNestedSelection();
                    removePrevParentSelection();
                    mSelectedParentItemPos = holder.getLayoutPosition();
                    AndroidUtils.show(holder.ivSelected);
                    network.setIsSelected(true);
                    if (networkClickListener != null) {
                        networkClickListener.onNetworkItemSelected(
                                mNetworkInfos.get(mSelectedParentItemPos).mNetworkInfo);
                    }
                };

                if (!network.getSubNetworksItems().isEmpty()) {
                    AndroidUtils.show(holder.ivSublistIcon);
                }

                if (network.isSelected()) {
                    AndroidUtils.show(holder.ivSelected);
                } else {
                    AndroidUtils.invisible(holder.ivSelected);
                }

                if (network.isShowingSublist) {
                    holder.ivSublistIcon.setImageResource(R.drawable.ic_up_icon);
                    NestedNetworkSelectorAdapter nestedAdapter =
                            network.mNestedNetworkSelectorAdapter;
                    if (nestedAdapter == null) {
                        nestedAdapter = new NestedNetworkSelectorAdapter(mContext,
                                network.mSubNetworksItems, nestedCallback,
                                holder.getLayoutPosition());
                        nestedAdapter.setOnNetworkItemSelected(this.networkClickListener);
                    }
                    holder.subNetworksRv.setAdapter(nestedAdapter);
                    network.mNestedNetworkSelectorAdapter = nestedAdapter;
                    AndroidUtils.show(holder.subNetworksRv);
                } else {
                    holder.ivSublistIcon.setImageResource(R.drawable.ic_down_icon);
                    AndroidUtils.gone(holder.subNetworksRv);
                }
                Utils.setBlockiesBitmapResource(mExecutor, mHandler, holder.ivNetworkPicture,
                        network.getNetworkName(), false);
                holder.ivSelected.setVisibility(
                        network.isSelected() ? View.VISIBLE : View.INVISIBLE);
                holder.ivNetworkPicture.setOnClickListener(listener);
                holder.tvName.setOnClickListener(listener);
                holder.ivSublistIcon.setOnClickListener(v -> {
                    network.isShowingSublist = !network.isShowingSublist;
                    notifyItemChanged(holder.getLayoutPosition());
                });
                break;
            }
            case SECONDARY:
            case ITEM: {
                Utils.setBlockiesBitmapResource(mExecutor, mHandler, holder.ivNetworkPicture,
                        network.getNetworkName(), false);
                holder.ivSelected.setVisibility(
                        network.isSelected() ? View.VISIBLE : View.INVISIBLE);
                holder.itemView.setOnClickListener(v -> {
                    removePrevParentSelection();
                    removePrevNestedSelection();
                    AndroidUtils.show(holder.ivSelected);
                    mSelectedParentItemPos = holder.getLayoutPosition();
                    network.setIsSelected(true);
                    if (networkClickListener != null) {
                        networkClickListener.onNetworkItemSelected(
                                mNetworkInfos.get(mSelectedParentItemPos).mNetworkInfo);
                    }
                });
                if (network.isSelected()) {
                    AndroidUtils.show(holder.ivSelected);
                } else {
                    AndroidUtils.invisible(holder.ivSelected);
                }
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
    public void addPrimaryNetwork(List<NetworkInfoPresenter> networkInfos) {
        Utils.removeIf(mNetworkInfos, network -> network.getType() == Type.PRIMARY);
        List<NetworkSelectorItem> primaryItems = new ArrayList<>();
        for (NetworkInfoPresenter networkInfo : networkInfos) {
            primaryItems.add(new NetworkSelectorItem(networkInfo.mNetworkInfo.chainName,
                    networkInfo.mNetworkInfo.symbolName, networkInfo.mNetworkInfo, Type.PRIMARY,
                    networkInfo.mSubNetworks));
        }
        // Add items at top
        mNetworkInfos.addAll(0, primaryItems);
        notifyDataSetChanged();
        if (mSelectedNetwork != null) {
            setSelectedNetwork(mSelectedNetwork);
        }
    }

    @SuppressLint("NotifyDataSetChanged")
    public void addSecondaryNetwork(List<NetworkInfoPresenter> networkInfos) {
        Utils.removeIf(mNetworkInfos,
                network -> network.getType() != Type.PRIMARY || network.getType() == Type.LABEL);
        mNetworkInfos.add(new NetworkSelectorItem(
                mContext.getString(R.string.brave_wallet_network_filter_secondary), "", null,
                Type.LABEL, Collections.emptyList()));
        for (NetworkInfoPresenter networkInfo : networkInfos) {
            mNetworkInfos.add(new NetworkSelectorItem(networkInfo.mNetworkInfo.chainName,
                    networkInfo.mNetworkInfo.symbolName, networkInfo.mNetworkInfo, Type.SECONDARY,
                    Collections.emptyList()));
        }
        notifyDataSetChanged();
        if (mSelectedNetwork != null) {
            setSelectedNetwork(mSelectedNetwork);
        }
    }

    public void setSelectedNetwork(String networkName) {
        mSelectedNetwork = networkName;
        for (int i = 0; i < mNetworkInfos.size(); i++) {
            NetworkSelectorItem networkSelectorItem = mNetworkInfos.get(i);
            if (networkSelectorItem.getNetworkName().equalsIgnoreCase(networkName)) {
                removePrevParentSelection();
                removePrevNestedSelection();
                networkSelectorItem.setIsSelected(true);
                mSelectedParentItemPos = i;
                notifyItemChanged(mSelectedParentItemPos);
                break;
            }
            if (!networkSelectorItem.getSubNetworksItems().isEmpty()) {
                List<NetworkSelectorItem> nestedItems = networkSelectorItem.getSubNetworksItems();
                for (int j = 0; j < nestedItems.size(); j++) {
                    NetworkSelectorItem nestedItem = nestedItems.get(j);
                    if (networkName.equals(nestedItem.getNetworkName())) {
                        nestedItem.setIsSelected(true);
                        if (networkSelectorItem.mNestedNetworkSelectorAdapter != null) {
                            networkSelectorItem.mNestedNetworkSelectorAdapter.notifyItemChanged(j);
                        } else {
                            notifyItemChanged(i);
                        }
                        mSelectedNestedItemPos = Pair.create(i, j);
                        break;
                    }
                }
            }
        }
    }

    public void setOnNetworkItemSelected(NetworkClickListener networkClickListener) {
        this.networkClickListener = networkClickListener;
    }

    private void removePrevNestedSelection() {
        int parent = mSelectedNestedItemPos.first;
        int child = mSelectedNestedItemPos.second;
        NetworkSelectorItem parentItem = mNetworkInfos.get(parent);
        if (parentItem != null && !parentItem.getSubNetworksItems().isEmpty()) {
            NetworkSelectorItem childItem = parentItem.getSubNetworksItems().get(child);
            if (childItem.isSelected()) {
                childItem.setIsSelected(false);
                if (parentItem.mNestedNetworkSelectorAdapter != null) {
                    parentItem.mNestedNetworkSelectorAdapter.notifyItemChanged(child);
                }
            }
        }
    }

    private void removePrevParentSelection() {
        if (mNetworkInfos.get(mSelectedParentItemPos).isSelected()) {
            mNetworkInfos.get(mSelectedParentItemPos).setIsSelected(false);
            notifyItemChanged(mSelectedParentItemPos);
        }
    }

    enum Type { ITEM, PRIMARY, SECONDARY, LABEL }

    public interface NetworkClickListener {
        void onNetworkItemSelected(NetworkInfo networkInfo);
    }

    static class ViewHolder extends RecyclerView.ViewHolder {
        ImageView ivNetworkPicture;
        TextView tvName;
        ImageView ivSelected;
        ImageView ivSublistIcon;
        RecyclerView subNetworksRv;
        TextView labelTv;

        ViewHolder(View itemView) {
            super(itemView);
            tvName = itemView.findViewById(R.id.tv_item_network_name);
            ivNetworkPicture = itemView.findViewById(R.id.iv_item_network_picture);
            ivSelected = itemView.findViewById(R.id.iv_item_network_selector_selected);
            ivSublistIcon = itemView.findViewById(R.id.iv_item_network_selector_sublist_icon);
            subNetworksRv = itemView.findViewById(R.id.iv_item_sublist);
            labelTv = itemView.findViewById(R.id.iv_item_network_label);
        }
    }

    public static class NetworkSelectorItem {
        private NetworkInfo mNetworkInfo;
        private boolean isShowingSublist;
        private NestedNetworkSelectorAdapter mNestedNetworkSelectorAdapter;
        private String mNetworkName;
        private String mNetworkShortName;
        private boolean mIsSelected;
        private Type mType;
        private List<NetworkSelectorItem> mSubNetworksItems;

        public NetworkSelectorItem(
                String networkName, String networkShortName, NetworkInfo networkInfo) {
            this.mNetworkName = networkName;
            this.mNetworkShortName = networkShortName;
            this.mNetworkInfo = networkInfo;
        }

        public NetworkSelectorItem(String networkName, String networkShortName,
                NetworkInfo networkInfo, Type type, List<NetworkInfo> subNetworks) {
            this(networkName, networkShortName, networkInfo);
            this.mType = type;
            if (mType == null) {
                mType = Type.ITEM;
            }
            mSubNetworksItems = initSubItems(subNetworks);
        }

        private List<NetworkSelectorItem> initSubItems(List<NetworkInfo> networkInfos) {
            List<NetworkSelectorItem> items = new ArrayList<>();
            for (NetworkInfo networkInfo : networkInfos) {
                items.add(new NetworkSelectorItem(
                        networkInfo.chainName, networkInfo.symbolName, networkInfo));
            }
            return items;
        }

        public List<NetworkSelectorItem> getSubNetworksItems() {
            return mSubNetworksItems;
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

    static class NestedNetworkSelectorAdapter
            extends RecyclerView.Adapter<NestedNetworkSelectorAdapter.NestedViewHolder> {
        private final ExecutorService mExecutor;
        private final Handler mHandler;
        private final LayoutInflater inflater;
        private Context mContext;
        private Callbacks.Callback2<Integer, Integer> mOnNestedSelectedClick;
        private int mParentPos;
        private int previousSelectedPos;
        private NetworkClickListener networkClickListener;
        private List<NetworkSelectorItem> mNestedNetworkInfos;

        public NestedNetworkSelectorAdapter(Context context, List<NetworkSelectorItem> networkInfos,
                Callbacks.Callback2<Integer, Integer> onNestedSelectedClick, int parentPos) {
            this.mContext = context;
            inflater = (LayoutInflater.from(context));
            mOnNestedSelectedClick = onNestedSelectedClick;
            mParentPos = parentPos;
            mExecutor = Executors.newSingleThreadExecutor();
            mHandler = new Handler(Looper.getMainLooper());
            mNestedNetworkInfos = networkInfos;
        }

        @Override
        public @NonNull NestedViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
            mContext = parent.getContext();
            View view = inflater.inflate(R.layout.item_network_selector, parent, false);
            return new NestedViewHolder(view);
        }

        @SuppressLint("NotifyDataSetChanged")
        @Override
        public void onBindViewHolder(@NonNull NestedViewHolder holder, int position) {
            final NetworkSelectorItem network = mNestedNetworkInfos.get(position);

            holder.tvName.setText(network.getNetworkName());
            Utils.setBlockiesBitmapResource(
                    mExecutor, mHandler, holder.ivNetworkPicture, network.getNetworkName(), false);
            holder.ivSelected.setVisibility(network.isSelected() ? View.VISIBLE : View.INVISIBLE);

            holder.itemView.setOnClickListener(v -> {
                network.setIsSelected(true);
                AndroidUtils.show(holder.ivSelected);
                if (mOnNestedSelectedClick != null) {
                    mOnNestedSelectedClick.call(mParentPos, position);
                }
                if (networkClickListener != null) {
                    networkClickListener.onNetworkItemSelected(
                            mNestedNetworkInfos.get(position).mNetworkInfo);
                }
            });
        }

        @Override
        public int getItemCount() {
            return mNestedNetworkInfos.size();
        }

        public void setOnNetworkItemSelected(NetworkClickListener networkClickListener) {
            this.networkClickListener = networkClickListener;
        }

        class NestedViewHolder extends RecyclerView.ViewHolder {
            ImageView ivNetworkPicture;
            TextView tvName;
            ImageView ivSelected;

            NestedViewHolder(View itemView) {
                super(itemView);
                tvName = itemView.findViewById(R.id.tv_item_network_name);
                ivNetworkPicture = itemView.findViewById(R.id.iv_item_network_picture);
                ivSelected = itemView.findViewById(R.id.iv_item_network_selector_selected);
            }
        }
    }
}
