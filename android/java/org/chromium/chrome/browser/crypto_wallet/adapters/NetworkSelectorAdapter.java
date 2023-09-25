/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.adapters;

import android.annotation.SuppressLint;
import android.content.Context;
import android.graphics.ColorMatrix;
import android.graphics.ColorMatrixColorFilter;
import android.os.Handler;
import android.os.Looper;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.domain.NetworkModel;
import org.chromium.chrome.browser.app.domain.NetworkSelectorModel;
import org.chromium.chrome.browser.crypto_wallet.BlockchainRegistryFactory;
import org.chromium.chrome.browser.crypto_wallet.util.AndroidUtils;
import org.chromium.chrome.browser.crypto_wallet.util.NetworkUtils;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.stream.Collectors;

public class NetworkSelectorAdapter
        extends RecyclerView.Adapter<NetworkSelectorAdapter.ViewHolder> {
    private final ExecutorService mExecutor;
    private final Handler mHandler;
    private final LayoutInflater inflater;
    private final String mTokensPath;
    private Context mContext;
    private NetworkClickListener networkClickListener;
    private List<NetworkSelectorItem> mNetworkInfos;
    private List<NetworkInfo> mSelectedNetworks;
    private int mSelectedParentItemPos;
    private NetworkSelectorModel.SelectionMode mSelectionMode;
    private Button mBtnSelection;

    private Set<Integer> mSelectedNetworkPositions;

    public NetworkSelectorAdapter(Context context, List<NetworkSelectorItem> networkInfos,
            NetworkSelectorModel.SelectionMode selectionMode) {
        mNetworkInfos = networkInfos;
        this.mContext = context;
        inflater = (LayoutInflater.from(context));
        mExecutor = Executors.newSingleThreadExecutor();
        mHandler = new Handler(Looper.getMainLooper());
        mTokensPath = BlockchainRegistryFactory.getInstance().getTokensIconsLocation();
        mSelectionMode = selectionMode;
        mSelectedNetworkPositions = new HashSet<>();
        mSelectedNetworks = Collections.emptyList();
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
        holder.ivNetworkPicture.clearColorFilter();

        switch (network.mType) {
            case SECONDARY:
            case TEST:
            case PRIMARY: {
                View.OnClickListener listener = v -> {
                    mSelectedParentItemPos = holder.getLayoutPosition();
                    if (NetworkSelectorModel.SelectionMode.MULTI == mSelectionMode) {
                        if (network.mIsSelected) {
                            mSelectedNetworkPositions.remove(mSelectedParentItemPos);
                        } else {
                            mSelectedNetworkPositions.add(mSelectedParentItemPos);
                        }
                        network.setIsSelected(!network.isSelected());
                        updateAllSelectionLabel();
                        notifyItemChanged(mSelectedParentItemPos);
                    } else {
                        // Handle single network selection type
                        removePrevSelection();
                        AndroidUtils.show(holder.ivSelected);
                        network.setIsSelected(true);
                        if (networkClickListener != null) {
                            networkClickListener.onNetworkItemSelected(
                                    mNetworkInfos.get(mSelectedParentItemPos).mNetworkInfo);
                        }
                    }
                };
                if (network.isSelected()) {
                    AndroidUtils.show(holder.ivSelected);
                } else {
                    AndroidUtils.invisible(holder.ivSelected);
                }

                String logo = Utils.getNetworkIconName(
                        network.mNetworkInfo.chainId, network.mNetworkInfo.coin);
                if (!TextUtils.isEmpty(logo)) {
                    Utils.setBitmapResource(mExecutor, mHandler, mContext,
                            "file://" + mTokensPath + "/" + logo, Integer.MIN_VALUE,
                            holder.ivNetworkPicture, null, true);
                    if (NetworkUtils.isTestNetwork(network.mNetworkInfo.chainId)) {
                        // Grey style test net image
                        ColorMatrix matrix = new ColorMatrix();
                        matrix.setSaturation(0);

                        ColorMatrixColorFilter filter = new ColorMatrixColorFilter(matrix);
                        holder.ivNetworkPicture.setColorFilter(filter);
                    }
                } else if (!NetworkUtils.isAllNetwork(network.mNetworkInfo)) {
                    Utils.setTextGeneratedBlockies(mExecutor, mHandler, holder.ivNetworkPicture,
                            network.getNetworkName(), false);
                } else {
                    AndroidUtils.gone(holder.ivNetworkPicture);
                }
                holder.ivNetworkPicture.setOnClickListener(listener);
                holder.tvName.setOnClickListener(listener);
                break;
            }
            case LABEL: {
                AndroidUtils.gone(holder.ivNetworkPicture, holder.tvName, holder.ivNetworkPicture,
                        holder.ivSelected);
                if (position == 0 && NetworkSelectorModel.SelectionMode.MULTI == mSelectionMode) {
                    mBtnSelection = holder.btnAction;
                    AndroidUtils.show(mBtnSelection);
                    updateAllSelectionLabel();
                    mBtnSelection.setOnClickListener(v -> {
                        updateNetworksSelection(!isAllNetworkSelected());
                        updateAllSelectionLabel();
                    });
                }
                AndroidUtils.show(holder.labelTv);
                holder.labelTv.setText(network.getNetworkName());
                break;
            }
        }
    }

    private void updateAllSelectionLabel() {
        if (mBtnSelection == null) return;
        mBtnSelection.setText(isAllNetworkSelected() ? R.string.brave_wallet_deselect_all
                                                     : R.string.brave_wallet_select_all);
    }

    @Override
    public int getItemCount() {
        return mNetworkInfos.size();
    }

    @SuppressLint("NotifyDataSetChanged")
    public void addNetworks(NetworkModel.NetworkLists networkLists) {
        mNetworkInfos.clear();
        // Primary
        mNetworkInfos.add(new NetworkSelectorItem(
                mContext.getString(R.string.brave_wallet_network_filter_primary), "", null,
                Type.LABEL));
        for (NetworkInfo networkInfo : networkLists.mPrimaryNetworkList) {
            mNetworkInfos.add(new NetworkSelectorItem(
                    networkInfo.chainName, networkInfo.symbolName, networkInfo, Type.PRIMARY));
        }

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

        if (!mSelectedNetworks.isEmpty()) {
            setSelectedNetwork(mSelectedNetworks);
        }
        notifyDataSetChanged();
    }

    public void setSelectedNetwork(List<NetworkInfo> networkInfo) {
        mSelectedNetworks = networkInfo;
        if (mSelectedNetworks.isEmpty()) return;
        // Mark all network as selected in MULTI network selection mode when only AllNetwork is
        // passed
        if (NetworkSelectorModel.SelectionMode.MULTI == mSelectionMode
                && mSelectedNetworks.size() == 1 && NetworkUtils.isAllNetwork(networkInfo.get(0))) {
            for (int i = 0; i < mNetworkInfos.size(); i++) {
                NetworkSelectorItem networkSelectorItem = mNetworkInfos.get(i);
                if (Type.LABEL == networkSelectorItem.mType) continue;
                networkSelectorItem.setIsSelected(true);
                mSelectedNetworkPositions.add(i);
            }
            return;
        }
        List<String> selectedNetworks =
                networkInfo.stream().map(network -> network.chainId).collect(Collectors.toList());
        for (int i = 0; i < mNetworkInfos.size(); i++) {
            NetworkSelectorItem networkSelectorItem = mNetworkInfos.get(i);
            if (Type.LABEL != networkSelectorItem.mType
                    && selectedNetworks.contains(networkSelectorItem.mNetworkInfo.chainId)) {
                if (NetworkSelectorModel.SelectionMode.SINGLE == mSelectionMode) {
                    removePrevSelection();
                } else {
                    mSelectedNetworkPositions.add(i);
                }
                networkSelectorItem.setIsSelected(true);
                mSelectedParentItemPos = i;
                notifyItemChanged(mSelectedParentItemPos);
            }
        }
    }

    public void setOnNetworkItemSelected(NetworkClickListener networkClickListener) {
        this.networkClickListener = networkClickListener;
    }

    public List<NetworkInfo> getSelectedNetworks() {
        List<NetworkInfo> selectedNetworks = new ArrayList<>();
        for (int i = 0; i < mNetworkInfos.size(); i++) {
            NetworkSelectorItem networkItem = mNetworkInfos.get(i);
            if (mSelectedNetworkPositions.contains(i) && networkItem.mType != Type.LABEL) {
                selectedNetworks.add(networkItem.mNetworkInfo);
            }
        }
        return selectedNetworks;
    }

    private void removePrevSelection() {
        if (mNetworkInfos.get(mSelectedParentItemPos).isSelected()) {
            mNetworkInfos.get(mSelectedParentItemPos).setIsSelected(false);
            notifyItemChanged(mSelectedParentItemPos);
        }
    }

    private void updateNetworksSelection(boolean isAllSelected) {
        mSelectedNetworkPositions.clear();
        for (int i = 0; i < mNetworkInfos.size(); i++) {
            NetworkSelectorItem networkSelectorItem = mNetworkInfos.get(i);
            if (Type.LABEL != networkSelectorItem.mType) {
                networkSelectorItem.mIsSelected = isAllSelected;
            }
            if (isAllSelected) {
                mSelectedNetworkPositions.add(i);
            }
        }
        // Exclude first label
        notifyItemRangeChanged(1, mNetworkInfos.size());
    }

    private boolean isAllNetworkSelected() {
        if (mNetworkInfos.isEmpty()) return true;
        for (NetworkSelectorItem networkSelectorItem : mNetworkInfos) {
            if (Type.LABEL != networkSelectorItem.mType && !networkSelectorItem.mIsSelected) {
                return false;
            }
        }
        return true;
    }

    enum Type { PRIMARY, SECONDARY, TEST, LABEL }

    public interface NetworkClickListener {
        void onNetworkItemSelected(NetworkInfo networkInfo);
    }

    static class ViewHolder extends RecyclerView.ViewHolder {
        ImageView ivNetworkPicture;
        TextView tvName;
        ImageView ivSelected;
        TextView labelTv;
        Button btnAction;

        ViewHolder(View itemView) {
            super(itemView);
            tvName = itemView.findViewById(R.id.tv_item_network_name);
            ivNetworkPicture = itemView.findViewById(R.id.iv_item_network_picture);
            ivSelected = itemView.findViewById(R.id.iv_item_network_selector_selected);
            labelTv = itemView.findViewById(R.id.iv_item_network_label);
            btnAction = itemView.findViewById(R.id.btn_item_network_action);
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
