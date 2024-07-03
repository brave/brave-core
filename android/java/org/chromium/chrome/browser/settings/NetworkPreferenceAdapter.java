/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import static org.chromium.components.browser_ui.widget.BrowserUiListMenuUtils.buildMenuListItem;

import android.content.Context;
import android.graphics.Typeface;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import androidx.recyclerview.widget.RecyclerView;
import androidx.recyclerview.widget.RecyclerView.ViewHolder;

import org.chromium.base.Callback;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.util.WalletConstants;
import org.chromium.ui.listmenu.BasicListMenu;
import org.chromium.ui.listmenu.ListMenu;
import org.chromium.ui.listmenu.ListMenuButton;
import org.chromium.ui.listmenu.ListMenuButtonDelegate;
import org.chromium.ui.listmenu.ListMenuItemProperties;
import org.chromium.ui.modelutil.MVCListAdapter.ModelList;

import java.util.ArrayList;
import java.util.List;

/**
 * Network preference adapter that shows all the available networks and let the user add, edit or
 * remove a specific network. Used by {@link BraveWalletNetworksPreference}.
 */
public class NetworkPreferenceAdapter extends RecyclerView.Adapter<ViewHolder> {
    private static final String TAG = "NetworkPreference";

    /** Item types. */
    private static final int NETWORK_ITEM = 0;

    private static final int LABEL_ETHEREUM_ITEM = 1;
    private static final int LABEL_FILECOIN_ITEM = 2;
    private static final int LABEL_SOLANA_ITEM = 3;
    private static final int LABEL_BITCOIN_ITEM = 4;

    private final ItemClickListener mListener;
    private final Context mContext;
    // Network items including their header.
    private final List<NetworkPreferenceItem> mElements;

    /**
     * Listener implemented by {@link BraveWalletNetworksPreference} used to handle network
     * operations.
     */
    interface ItemClickListener {
        /** Triggered when a specific network is clicked to be modified. */
        void onItemEdit(@NonNull final NetworkInfo chain, final boolean activeNetwork);

        /** Triggered to remove a custom network completely. */
        void onItemRemove(
                @NonNull final NetworkInfo chain, @NonNull final Callback<Boolean> callback);

        /** Triggered when a network has been selected as active. */
        void onItemSetAsActive(
                @NonNull final NetworkInfo chain, @NonNull final Callback<Boolean> callback);

        void onItemVisibilityChange(@NonNull final NetworkInfo chain, final boolean hidden);
    }

    public NetworkPreferenceAdapter(
            @NonNull final Context context,
            @NonNull NetworkInfo[] networks,
            @NonNull final ItemClickListener listener) {
        mContext = context;

        List<NetworkInfo> ethNetworks = new ArrayList<>();
        List<NetworkInfo> solNetworks = new ArrayList<>();
        List<NetworkInfo> filNetworks = new ArrayList<>();
        List<NetworkInfo> btcNetworks = new ArrayList<>();
        for (NetworkInfo network : networks) {
            if (network.coin == CoinType.ETH) {
                ethNetworks.add(network);
            } else if (network.coin == CoinType.SOL) {
                solNetworks.add(network);
            } else if (network.coin == CoinType.FIL) {
                filNetworks.add(network);
            } else if (network.coin == CoinType.BTC) {
                btcNetworks.add(network);
            }
        }

        // Generate items for ETH networks.
        mElements = getEthNetworkInfoByChainId(ethNetworks);

        // Generate items for FIL networks.
        if (filNetworks.size() > 0) {
            mElements.add(new NetworkPreferenceItem(LABEL_FILECOIN_ITEM));
            for (NetworkInfo filNetwork : filNetworks) {
                mElements.add(new NetworkPreferenceItem(filNetwork));
            }
        }

        // Generate items for SOL networks.
        if (solNetworks.size() > 0) {
            mElements.add(new NetworkPreferenceItem(LABEL_SOLANA_ITEM));
            for (NetworkInfo solNetwork : solNetworks) {
                mElements.add(new NetworkPreferenceItem(solNetwork));
            }
        }

        // Generate items for BTC networks.
        if (btcNetworks.size() > 0) {
            mElements.add(new NetworkPreferenceItem(LABEL_BITCOIN_ITEM));
            for (NetworkInfo btcNetwork : btcNetworks) {
                mElements.add(new NetworkPreferenceItem(btcNetwork));
            }
        }

        mListener = listener;
    }

    @NonNull
    private List<NetworkPreferenceItem> getEthNetworkInfoByChainId(
            @NonNull final List<NetworkInfo> allNetworks) {
        final List<NetworkPreferenceItem> customChains = new ArrayList<>();
        final List<NetworkPreferenceItem> defaultChains = new ArrayList<>();
        final List<NetworkPreferenceItem> result = new ArrayList<>();
        for (NetworkInfo network : allNetworks) {
            if (network.props.isCustom) {
                customChains.add(new NetworkPreferenceItem(network));
            } else {
                defaultChains.add(new NetworkPreferenceItem(network));
            }
        }

        result.add(new NetworkPreferenceItem(LABEL_ETHEREUM_ITEM));
        result.addAll(defaultChains);
        result.addAll(customChains);
        return result;
    }

    @NonNull
    @Override
    public ViewHolder onCreateViewHolder(@NonNull ViewGroup viewGroup, int viewType) {
        final LayoutInflater inflater = LayoutInflater.from(viewGroup.getContext());
        if (viewType == NETWORK_ITEM) {
            View row = inflater.inflate(R.layout.brave_wallet_network_item, viewGroup, false);
            return new NetworkViewHolder(row);
        } else {
            // Label item.
            View view = inflater.inflate(R.layout.brave_wallet_network_label, viewGroup, false);
            return new LabelViewHolder(view);
        }
    }

    @Override
    public void onBindViewHolder(@NonNull ViewHolder viewHolder, int position) {
        final int type = viewHolder.getItemViewType();
        final NetworkPreferenceItem networkPreferenceItem = mElements.get(position);

        if (type == NETWORK_ITEM) {
            final NetworkInfo networkInfo = networkPreferenceItem.getNetworkInfo();
            final NetworkViewHolder networkViewHolder = (NetworkViewHolder) viewHolder;

            networkViewHolder.mTitle.setText(networkInfo.chainName);
            if (networkInfo.props.isDappDefault) {
                networkViewHolder.mTitle.setTypeface(null, Typeface.BOLD);
                networkViewHolder.mShowHideNetwork.setEnabled(false);
            } else {
                networkViewHolder.mTitle.setTypeface(null);
                networkViewHolder.mShowHideNetwork.setEnabled(true);
            }

            if (networkInfo.props.isHidden && !networkInfo.props.isDappDefault) {
                networkViewHolder.mShowHideNetwork.setImageResource(R.drawable.ic_eye_off);
            } else {
                networkViewHolder.mShowHideNetwork.setImageResource(R.drawable.ic_eye_on);
            }

            StringBuilder description = new StringBuilder(networkInfo.chainId);
            if (networkInfo.activeRpcEndpointIndex >= 0
                    && networkInfo.activeRpcEndpointIndex < networkInfo.rpcEndpoints.length) {
                description
                        .append(" ")
                        .append(networkInfo.rpcEndpoints[networkInfo.activeRpcEndpointIndex].url);
            }
            networkViewHolder.mDescription.setText(description.toString());

            networkViewHolder.mMoreButton.setContentDescriptionContext(networkInfo.chainName);

            // The more button will become visible if setMenuButtonDelegate is called.
            networkViewHolder.mMoreButton.setVisibility(View.INVISIBLE);

            // When a network is set as active, it cannot be removed or edited.
            if (networkInfo.props.isDappDefault) {
                return;
            }

            networkViewHolder.mShowHideNetwork.setOnClickListener(
                    v -> {
                        mListener.onItemVisibilityChange(networkInfo, !networkInfo.props.isHidden);

                        networkInfo.props.isHidden = !networkInfo.props.isHidden;
                        networkViewHolder.mShowHideNetwork.setImageResource(
                                networkInfo.props.isHidden
                                        ? R.drawable.ic_eye_off
                                        : R.drawable.ic_eye_on);
                        notifyItemChanged(position);
                    });

            final ModelList menuItems = new ModelList();
            if (networkInfo.props.isCustom) {
                menuItems.add(buildMenuListItem(R.string.edit, 0, 0));
                menuItems.add(buildMenuListItem(R.string.remove, 0, 0));
            }
            if (WalletConstants.SUPPORTED_COIN_TYPES_ON_DAPPS.contains(networkInfo.coin)) {
                menuItems.add(
                        buildMenuListItem(R.string.brave_wallet_add_network_set_as_active, 0, 0));
            }

            if (menuItems.size() == 0) {
                return;
            }

            ListMenu.Delegate delegate =
                    (model) -> {
                        int textId = model.get(ListMenuItemProperties.TITLE_ID);
                        if (textId == R.string.edit) {
                            mListener.onItemEdit(networkInfo, false);
                        } else if (textId == R.string.remove) {
                            // Remove option is only for ETH custom networks.
                            mListener.onItemRemove(
                                    networkInfo,
                                    result -> {
                                        if (result) {
                                            mElements.remove(networkPreferenceItem);
                                            notifyItemRemoved(position);
                                        }
                                    });
                        } else if (textId == R.string.brave_wallet_add_network_set_as_active) {
                            mListener.onItemSetAsActive(
                                    networkInfo,
                                    result -> {
                                        if (result) {
                                            setActiveChainId(networkInfo);
                                        }
                                    });
                        }
                    };
            networkViewHolder.setMenuButtonDelegate(
                    () -> {
                        View contentView =
                                LayoutInflater.from(mContext)
                                        .inflate(
                                                R.layout.app_menu_layout,
                                                networkViewHolder.mItem,
                                                false);
                        ListView listView = contentView.findViewById(R.id.app_menu_list);
                        return new BasicListMenu(
                                mContext, menuItems, contentView, listView, delegate, 0);
                    });
        } else {
            final LabelViewHolder labelViewHolder = (LabelViewHolder) viewHolder;
            labelViewHolder.mLabel.setText(networkPreferenceItem.getNetworkNameRes());
        }
    }

    private void setActiveChainId(@NonNull final NetworkInfo network) {
        for (int i = 0; i < mElements.size(); i++) {
            NetworkPreferenceItem networkPreferenceItem = mElements.get(i);
            NetworkInfo curNetwork = networkPreferenceItem.mNetworkInfo;
            if (curNetwork != null && network.coin == curNetwork.coin) {
                if (network.chainId.equals(curNetwork.chainId)) {
                    curNetwork.props.isDappDefault = true;
                    notifyItemChanged(i);
                } else {
                    if (curNetwork.props.isDappDefault) {
                        curNetwork.props.isDappDefault = false;
                        notifyItemChanged(i);
                    }
                }
            }
        }
    }

    @Override
    public int getItemCount() {
        return mElements.size();
    }

    @Override
    public int getItemViewType(int position) {
        return mElements.get(position).getType();
    }

    public static class LabelViewHolder extends RecyclerView.ViewHolder {
        final TextView mLabel;

        public LabelViewHolder(View itemView) {
            super(itemView);
            mLabel = itemView.findViewById(R.id.network_label);
        }
    }

    static class NetworkViewHolder extends ViewHolder {
        private final TextView mTitle;
        private final TextView mDescription;
        private final LinearLayout mItem;
        private final ImageView mShowHideNetwork;

        private final ListMenuButton mMoreButton;

        NetworkViewHolder(@NonNull final View view) {
            super(view);

            mTitle = view.findViewById(R.id.title);
            mDescription = view.findViewById(R.id.description);
            mShowHideNetwork = view.findViewById(R.id.show_hide_network);
            mMoreButton = view.findViewById(R.id.more);
            mItem = view.findViewById(R.id.network_item);
        }

        /**
         * Sets up the menu button at the end of this row with a given delegate.
         *
         * @param delegate A {@link ListMenuButtonDelegate}.
         */
        void setMenuButtonDelegate(@NonNull final ListMenuButtonDelegate delegate) {
            mMoreButton.setVisibility(View.VISIBLE);
            mMoreButton.setDelegate(delegate);
        }
    }

    public static class NetworkPreferenceItem {
        private final int mType;

        /** Network info is available only when type is {@code NETWORK_ITEM}. */
        @Nullable private final NetworkInfo mNetworkInfo;

        /** Network name resource is available only when type is not {@code NETWORK_ITEM} */
        private final int mNetworkNameRes;

        public NetworkPreferenceItem(final int type) {
            mType = type;
            mNetworkInfo = null;
            if (mType == LABEL_ETHEREUM_ITEM) {
                mNetworkNameRes = R.string.brave_ethereum_networks;
            } else if (mType == LABEL_FILECOIN_ITEM) {
                mNetworkNameRes = R.string.brave_filecoin_networks;
            } else if (mType == LABEL_SOLANA_ITEM) {
                mNetworkNameRes = R.string.brave_solana_networks;
            } else if (mType == LABEL_BITCOIN_ITEM) {
                mNetworkNameRes = R.string.brave_bitcoin_networks;
            } else {
                throw new IllegalStateException(
                        String.format("Network name not found for label type %d.", mType));
            }
        }

        public NetworkPreferenceItem(@NonNull final NetworkInfo networkInfo) {
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
