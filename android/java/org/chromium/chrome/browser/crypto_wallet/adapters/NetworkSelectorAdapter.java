/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

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
import org.chromium.chrome.browser.crypto_wallet.util.Utils;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class NetworkSelectorAdapter
        extends RecyclerView.Adapter<NetworkSelectorAdapter.ViewHolder> {
    private Context mContext;
    private final ExecutorService mExecutor;
    private final Handler mHandler;
    private final List<NetworkSelectorItem> networks;
    private final LayoutInflater inflater;
    private int previousSelectedPos;
    private NetworkClickListener networkClickListener;
    private NetworkInfo[] mNetworkInfos;

    public NetworkSelectorAdapter(Context context, NetworkInfo[] networkInfos) {
        mNetworkInfos = networkInfos;
        this.mContext = context;
        inflater = (LayoutInflater.from(context));
        mExecutor = Executors.newSingleThreadExecutor();
        mHandler = new Handler(Looper.getMainLooper());
        networks = new ArrayList<>();
        init(mNetworkInfos);
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
        final NetworkSelectorItem network = networks.get(position);

        holder.tvName.setText(network.getNetworkName());
        Utils.setBlockiesBitmapResource(
                mExecutor, mHandler, holder.ivNetworkPicture, network.getNetworkName(), false);
        holder.ivSelected.setVisibility(network.isSelected() ? View.VISIBLE : View.GONE);

        holder.itemView.setOnClickListener(v -> {
            updateSelectedNetwork(holder.getLayoutPosition());
            if (networkClickListener != null) {
                networkClickListener.onNetworkItemSelected(mNetworkInfos[position]);
            }
        });
    }

    @Override
    public int getItemCount() {
        return networks.size();
    }

    public void setSelectedNetwork(String network) {
        for (int i = 0; i < networks.size(); i++) {
            NetworkSelectorItem networkSelectorItem = networks.get(i);
            if (networkSelectorItem.getNetworkName().equalsIgnoreCase(network)) {
                updateSelectedNetwork(i);
                break;
            }
        }
    }

    public void setOnNetworkItemSelected(NetworkClickListener networkClickListener) {
        this.networkClickListener = networkClickListener;
    }

    private void updateSelectedNetwork(int selectedNetworkPosition) {
        networks.get(previousSelectedPos).setIsSelected(false);
        notifyItemChanged(previousSelectedPos);

        networks.get(selectedNetworkPosition).setIsSelected(true);
        previousSelectedPos = selectedNetworkPosition;
        notifyItemChanged(selectedNetworkPosition);
    }

    private void init(NetworkInfo[] networkInfos) {
        for (NetworkInfo networkInfo : networkInfos) {
            networks.add(new NetworkSelectorItem(networkInfo.chainName, networkInfo.symbolName));
        }
    }

    static class ViewHolder extends RecyclerView.ViewHolder {
        ImageView ivNetworkPicture;
        TextView tvName;
        ImageView ivSelected;

        ViewHolder(View itemView) {
            super(itemView);
            tvName = itemView.findViewById(R.id.tv_item_network_name);
            ivNetworkPicture = itemView.findViewById(R.id.iv_item_network_picture);
            ivSelected = itemView.findViewById(R.id.iv_item_network_selector_selected);
        }
    }

    public static class NetworkSelectorItem {
        private String mNetworkName;
        private String mNetworkShortName;
        private boolean mIsSelected;

        public NetworkSelectorItem(String mNetworkName, String mNetworkShortName) {
            this.mNetworkName = mNetworkName;
            this.mNetworkShortName = mNetworkShortName;
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

    public interface NetworkClickListener {
        void onNetworkItemSelected(NetworkInfo networkInfo);
    }
}
