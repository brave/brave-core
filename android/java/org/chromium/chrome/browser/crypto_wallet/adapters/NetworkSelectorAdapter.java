/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.adapters;

import android.content.Context;
import android.graphics.ColorMatrix;
import android.graphics.ColorMatrixColorFilter;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.DrawableRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.domain.NetworkModel;
import org.chromium.chrome.browser.crypto_wallet.activities.NetworkSelectorActivity;
import org.chromium.chrome.browser.crypto_wallet.util.NetworkUtils;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;

import java.util.ArrayList;
import java.util.List;

/**
 * Network selector adapter used by {@link NetworkSelectorActivity} that shows networks supporting
 * DApps. The adapter also contains three labels for primary, secondary and test networks. Labels
 * are shown only if there is at least one network. Tapping on a network will switch the selected
 * network and will finish the activity by calling {@link
 * NetworkClickListener#onNetworkItemSelected}.
 */
public class NetworkSelectorAdapter extends RecyclerView.Adapter<RecyclerView.ViewHolder> {

    /** Item types. */
    private static final int NETWORK_ITEM = 0;

    private static final int LABEL_PRIMARY_ITEM = 1;
    private static final int LABEL_SECONDARY_ITEM = 2;
    private static final int LABEL_TEST_ITEM = 3;

    private final LayoutInflater mInflater;
    private final NetworkClickListener mNetworkClickListener;
    private final List<NetworkSelectorItem> mNetworkSelectorItems;

    private int mSelectedNetworkIndex = -1;

    public NetworkSelectorAdapter(
            @NonNull final Context context,
            @NonNull final NetworkModel.NetworkLists networks,
            @NonNull final NetworkInfo selectedNetwork,
            @NonNull final NetworkClickListener networkClickListener) {
        mInflater = LayoutInflater.from(context);
        mNetworkClickListener = networkClickListener;

        int count = 0;
        mNetworkSelectorItems = new ArrayList<>();
        if (networks.mPrimaryNetworkList.size() > 0) {
            mNetworkSelectorItems.add(new NetworkSelectorItem(LABEL_PRIMARY_ITEM));
            count++;
            for (NetworkInfo networkInfo : networks.mPrimaryNetworkList) {
                mNetworkSelectorItems.add(new NetworkSelectorItem(networkInfo));
                if (mSelectedNetworkIndex == -1
                        && NetworkUtils.areEqual(selectedNetwork, networkInfo)) {
                    mSelectedNetworkIndex = count;
                }
                count++;
            }
        }

        if (networks.mSecondaryNetworkList.size() > 0) {
            mNetworkSelectorItems.add(new NetworkSelectorItem(LABEL_SECONDARY_ITEM));
            count++;
            for (NetworkInfo networkInfo : networks.mSecondaryNetworkList) {
                mNetworkSelectorItems.add(new NetworkSelectorItem(networkInfo));
                if (mSelectedNetworkIndex == -1
                        && NetworkUtils.areEqual(selectedNetwork, networkInfo)) {
                    mSelectedNetworkIndex = count;
                }
                count++;
            }
        }

        if (networks.mTestNetworkList.size() > 0) {
            mNetworkSelectorItems.add(new NetworkSelectorItem(LABEL_TEST_ITEM));
            count++;
            for (NetworkInfo networkInfo : networks.mTestNetworkList) {
                mNetworkSelectorItems.add(new NetworkSelectorItem(networkInfo));
                if (mSelectedNetworkIndex == -1
                        && NetworkUtils.areEqual(selectedNetwork, networkInfo)) {
                    mSelectedNetworkIndex = count;
                }
                count++;
            }
        }
    }

    @Override
    public int getItemViewType(int position) {
        return mNetworkSelectorItems.get(position).getType();
    }

    @Override
    public @NonNull RecyclerView.ViewHolder onCreateViewHolder(
            @NonNull ViewGroup parent, int viewType) {
        if (viewType == NETWORK_ITEM) {
            View view = mInflater.inflate(R.layout.item_network_selector, parent, false);
            return new NetworkViewHolder(
                    view,
                    position -> {
                        final int oldPosition = mSelectedNetworkIndex;
                        mSelectedNetworkIndex = position;
                        notifyItemChanged(oldPosition);
                        notifyItemChanged(mSelectedNetworkIndex);
                        final NetworkSelectorItem networkSelectorItem =
                                mNetworkSelectorItems.get(mSelectedNetworkIndex);
                        mNetworkClickListener.onNetworkItemSelected(
                                networkSelectorItem.getNetworkInfo());
                    });
        } else {
            // Label item.
            View view = mInflater.inflate(R.layout.label_network_selector, parent, false);
            return new LabelViewHolder(view);
        }
    }

    @Override
    public void onBindViewHolder(@NonNull RecyclerView.ViewHolder holder, int position) {
        final int type = holder.getItemViewType();
        final NetworkSelectorItem networkSelectorItem = mNetworkSelectorItems.get(position);

        if (type == NETWORK_ITEM) {
            final NetworkInfo network = networkSelectorItem.getNetworkInfo();
            final NetworkViewHolder networkViewHolder = (NetworkViewHolder) holder;

            networkViewHolder.mName.setText(network.chainName);
            networkViewHolder.mSelectedIcon.setVisibility(
                    mSelectedNetworkIndex == position ? View.VISIBLE : View.INVISIBLE);

            @DrawableRes int logo = Utils.getNetworkIconDrawable(network.chainId, network.coin);
            if (logo != -1) {
                networkViewHolder.mNetworkLogo.setVisibility(View.VISIBLE);
                networkViewHolder.mNetworkLogo.setImageResource(logo);
                if (NetworkUtils.isTestNetwork(network.chainId)) {
                    // Grey style test net image.
                    ColorMatrix matrix = new ColorMatrix();
                    matrix.setSaturation(0);

                    ColorMatrixColorFilter filter = new ColorMatrixColorFilter(matrix);
                    networkViewHolder.mNetworkLogo.setColorFilter(filter);
                } else {
                    networkViewHolder.mNetworkLogo.setColorFilter(null);
                }
            } else {
                networkViewHolder.mNetworkLogo.setVisibility(View.INVISIBLE);
                networkViewHolder.mNetworkLogo.setColorFilter(null);
            }
        } else {
            final LabelViewHolder labelViewHolder = (LabelViewHolder) holder;
            labelViewHolder.mLabel.setText(networkSelectorItem.getNetworkNameRes());
        }
    }

    @Override
    public int getItemCount() {
        return mNetworkSelectorItems.size();
    }

    public interface NetworkClickListener {
        void onNetworkItemSelected(@NonNull final NetworkInfo networkInfo);
    }

    public static class NetworkViewHolder extends RecyclerView.ViewHolder {
        interface OnClickListener {
            void onClick(final int position);
        }

        final ImageView mNetworkLogo;
        final TextView mName;
        final ImageView mSelectedIcon;

        public NetworkViewHolder(
                @NonNull final View itemView, @NonNull final OnClickListener listener) {
            super(itemView);
            itemView.setClickable(true);
            mName = itemView.findViewById(R.id.tv_item_network_name);
            mNetworkLogo = itemView.findViewById(R.id.iv_item_network_picture);
            mSelectedIcon = itemView.findViewById(R.id.iv_item_network_selector_selected);

            itemView.setOnClickListener(
                    view -> {
                        if (getBindingAdapterPosition() != RecyclerView.NO_POSITION) {
                            listener.onClick(getBindingAdapterPosition());
                        }
                    });
        }
    }

    public static class LabelViewHolder extends RecyclerView.ViewHolder {
        final TextView mLabel;

        public LabelViewHolder(View itemView) {
            super(itemView);
            mLabel = itemView.findViewById(R.id.iv_item_network_label);
        }
    }

    public static class NetworkSelectorItem {
        private final int mType;

        /** Network info is available only when type is {@code NETWORK_ITEM}. */
        @Nullable private final NetworkInfo mNetworkInfo;

        /** Network name resource is available only when type is not {@code NETWORK_ITEM} */
        private final int mNetworkNameRes;

        public NetworkSelectorItem(final int type) {
            mType = type;
            mNetworkInfo = null;
            if (mType == LABEL_PRIMARY_ITEM) {
                mNetworkNameRes = R.string.brave_wallet_network_filter_primary;
            } else if (mType == LABEL_SECONDARY_ITEM) {
                mNetworkNameRes = R.string.brave_wallet_network_filter_secondary;
            } else if (mType == LABEL_TEST_ITEM) {
                mNetworkNameRes = R.string.brave_wallet_network_filter_test;
            } else {
                throw new IllegalStateException(
                        String.format("Network name not found for label type %d.", mType));
            }
        }

        public NetworkSelectorItem(@NonNull final NetworkInfo networkInfo) {
            mType = NETWORK_ITEM;
            mNetworkInfo = networkInfo;
            mNetworkNameRes = 0;
        }

        public int getType() {
            return mType;
        }

        @NonNull
        public NetworkInfo getNetworkInfo() {
            if (mType != NETWORK_ITEM || mNetworkInfo == null) {
                throw new IllegalStateException(
                        "Network info can be retrieved only for network items.");
            }
            return mNetworkInfo;
        }

        @StringRes
        public int getNetworkNameRes() {
            if (mType == NETWORK_ITEM || mNetworkNameRes == 0) {
                throw new IllegalStateException(
                        "Network name can be retrieved only for label items.");
            }
            return mNetworkNameRes;
        }
    }
}
