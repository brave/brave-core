/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import static androidx.recyclerview.widget.RecyclerView.NO_POSITION;

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
import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.chrome.R;
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

    // ETH network types.
    private String mActiveEthChainId;
    private final List<String> mCustomEthChainIds;
    private final List<String> mHiddenEthChainIds;

    // FIL network types.
    private String mActiveFilChainId;
    private final List<String> mHiddenFilChainIds;

    // SOL network types.
    private String mActiveSolChainId;
    private final List<String> mHiddenSolChainIds;

    // BTC network types.
    private String mActiveBtcChainId;
    private final List<String> mHiddenBtcChainIds;

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

        void onItemVisibilityChange(
                @NonNull final NetworkInfo chain,
                final boolean show,
                @NonNull final Callback<Boolean> callback);
    }

    public NetworkPreferenceAdapter(
            @NonNull final Context context,
            @NonNull NetworkListContainer networkListContainerEth,
            @NonNull NetworkListContainer networkListContainerFil,
            @NonNull NetworkListContainer networkListContainerSol,
            @NonNull NetworkListContainer networkListContainerBtc,
            @NonNull final ItemClickListener listener) {
        mContext = context;

        mActiveEthChainId = networkListContainerEth.mActiveChainId;
        mCustomEthChainIds = networkListContainerEth.mCustomChainIds;
        mHiddenEthChainIds = networkListContainerEth.mHiddenChainIds;

        mActiveFilChainId = networkListContainerFil.mActiveChainId;
        mHiddenFilChainIds = networkListContainerFil.mHiddenChainIds;

        mActiveSolChainId = networkListContainerSol.mActiveChainId;
        mHiddenSolChainIds = networkListContainerSol.mHiddenChainIds;

        mActiveBtcChainId = networkListContainerBtc.mActiveChainId;
        mHiddenBtcChainIds = networkListContainerBtc.mHiddenChainIds;

        // Generate items for ETH networks.
        mElements =
                getEthNetworkInfoByChainId(mCustomEthChainIds, networkListContainerEth.mNetworks);

        // Generate items for FIL networks.
        if (networkListContainerFil.mNetworks.size() > 0) {
            mElements.add(new NetworkPreferenceItem(LABEL_FILECOIN_ITEM));
            for (NetworkInfo filNetwork : networkListContainerFil.mNetworks) {
                mElements.add(new NetworkPreferenceItem(filNetwork));
            }
        }

        // Generate items for SOL networks.
        if (networkListContainerSol.mNetworks.size() > 0) {
            mElements.add(new NetworkPreferenceItem(LABEL_SOLANA_ITEM));
            for (NetworkInfo solNetwork : networkListContainerSol.mNetworks) {
                mElements.add(new NetworkPreferenceItem(solNetwork));
            }
        }

        // Generate items for BTC networks.
        if (networkListContainerBtc.mNetworks.size() > 0) {
            mElements.add(new NetworkPreferenceItem(LABEL_BITCOIN_ITEM));
            for (NetworkInfo btcNetwork : networkListContainerBtc.mNetworks) {
                mElements.add(new NetworkPreferenceItem(btcNetwork));
            }
        }

        mListener = listener;
    }

    @NonNull
    private List<NetworkPreferenceItem> getEthNetworkInfoByChainId(
            @NonNull final List<String> customChainIds,
            @NonNull final List<NetworkInfo> allNetworks) {
        final List<NetworkPreferenceItem> customChains = new ArrayList<>();
        final List<NetworkPreferenceItem> defaultChains = new ArrayList<>();
        final List<NetworkPreferenceItem> result = new ArrayList<>();
        for (NetworkInfo network : allNetworks) {
            if (customChainIds.contains(network.chainId)) {
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
            final boolean activeNetwork = isActiveNetwork(networkInfo);
            // Custom networks are available only for ETH coin type.
            final boolean customNetwork = mCustomEthChainIds.contains(networkInfo.chainId);
            final NetworkViewHolder networkViewHolder = (NetworkViewHolder) viewHolder;
            final boolean visible;

            networkViewHolder.mTitle.setText(networkInfo.chainName);
            if (activeNetwork) {
                networkViewHolder.mTitle.setTypeface(null, Typeface.BOLD);
                networkViewHolder.mShowHideNetwork.setEnabled(false);
            } else {
                networkViewHolder.mTitle.setTypeface(null);
                networkViewHolder.mShowHideNetwork.setEnabled(true);
            }

            final boolean hiddenNetwork = isHiddenNetwork(networkInfo);

            if (hiddenNetwork && !activeNetwork) {
                visible = false;
                networkViewHolder.mShowHideNetwork.setImageResource(R.drawable.ic_eye_off);
            } else {
                visible = true;
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
            if (activeNetwork) {
                return;
            }

            networkViewHolder.mShowHideNetwork.setOnClickListener(
                    v ->
                            mListener.onItemVisibilityChange(
                                    networkInfo,
                                    !visible,
                                    result -> {
                                        if (result) {
                                            if (visible) {
                                                addHiddenNetwork(networkInfo);
                                                networkViewHolder.mShowHideNetwork.setImageResource(
                                                        R.drawable.ic_eye_off);
                                            } else {
                                                removeHiddenNetwork(networkInfo);
                                                networkViewHolder.mShowHideNetwork.setImageResource(
                                                        R.drawable.ic_eye_on);
                                            }
                                            notifyItemChanged(position);
                                        }
                                    }));

            final ModelList menuItems = new ModelList();
            if (customNetwork) {
                menuItems.add(buildMenuListItem(R.string.edit, 0, 0));
                menuItems.add(buildMenuListItem(R.string.remove, 0, 0));
            }
            menuItems.add(buildMenuListItem(R.string.brave_wallet_add_network_set_as_active, 0, 0));

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
                                            mCustomEthChainIds.remove(networkInfo.chainId);
                                            // The network may or may not be hidden.
                                            mHiddenEthChainIds.remove(networkInfo.chainId);
                                            mElements.remove(networkPreferenceItem);
                                            notifyItemRemoved(position);
                                        }
                                    });
                        } else if (textId == R.string.brave_wallet_add_network_set_as_active) {
                            mListener.onItemSetAsActive(
                                    networkInfo,
                                    result -> {
                                        if (result) {
                                            final String previousActiveChainId =
                                                    getActiveChainId(networkInfo.coin);
                                            setActiveChainId(networkInfo);

                                            final int oldActiveChainIdIndex =
                                                    findChainIdPosition(
                                                            previousActiveChainId,
                                                            networkInfo.coin);
                                            final int newActiveChainIdIndex =
                                                    findChainIdPosition(
                                                            networkInfo.chainId, networkInfo.coin);

                                            if (oldActiveChainIdIndex != NO_POSITION) {
                                                notifyItemChanged(oldActiveChainIdIndex);
                                            }

                                            if (newActiveChainIdIndex != NO_POSITION) {
                                                notifyItemChanged(newActiveChainIdIndex);
                                            }
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

    private boolean isHiddenNetwork(@NonNull final NetworkInfo network) {
        return (network.coin == CoinType.ETH && mHiddenEthChainIds.contains(network.chainId))
                || (network.coin == CoinType.FIL && mHiddenFilChainIds.contains(network.chainId))
                || (network.coin == CoinType.SOL && mHiddenSolChainIds.contains(network.chainId))
                || (network.coin == CoinType.BTC && mHiddenBtcChainIds.contains(network.chainId));
    }

    private boolean isActiveNetwork(@NonNull final NetworkInfo network) {
        return (network.coin == CoinType.ETH && network.chainId.equals(mActiveEthChainId))
                || (network.coin == CoinType.FIL && network.chainId.equals(mActiveFilChainId))
                || (network.coin == CoinType.SOL && network.chainId.equals(mActiveSolChainId))
                || (network.coin == CoinType.BTC && network.chainId.equals(mActiveBtcChainId));
    }

    private int findChainIdPosition(
            @Nullable final String chainId, @CoinType.EnumType final int coinType) {
        if (chainId == null) {
            return NO_POSITION;
        }
        for (int i = 0; i < mElements.size(); i++) {
            NetworkPreferenceItem networkPreferenceItem = mElements.get(i);
            if (networkPreferenceItem.mNetworkInfo != null
                    && chainId.equals(networkPreferenceItem.mNetworkInfo.chainId)
                    && coinType == networkPreferenceItem.mNetworkInfo.coin) {
                return i;
            }
        }
        return NO_POSITION;
    }

    @Nullable
    private String getActiveChainId(@CoinType.EnumType final int coinType) {
        String activeChainId = null;
        switch (coinType) {
            case CoinType.ETH -> activeChainId = mActiveEthChainId;
            case CoinType.FIL -> activeChainId = mActiveFilChainId;
            case CoinType.SOL -> activeChainId = mActiveSolChainId;
            case CoinType.BTC -> activeChainId = mActiveBtcChainId;
            case CoinType.ZEC -> Log.e(TAG, "Unsupported active chain for coin type: " + coinType);
        }
        return activeChainId;
    }

    private void setActiveChainId(@NonNull final NetworkInfo network) {
        switch (network.coin) {
            case CoinType.ETH -> mActiveEthChainId = network.chainId;
            case CoinType.FIL -> mActiveFilChainId = network.chainId;
            case CoinType.SOL -> mActiveSolChainId = network.chainId;
            case CoinType.BTC -> mActiveBtcChainId = network.chainId;
            case CoinType.ZEC -> Log.e(
                    TAG, "Unsupported active chain for coin type: " + network.coin);
        }
    }

    private void addHiddenNetwork(@NonNull final NetworkInfo network) {
        switch (network.coin) {
            case CoinType.ETH -> {
                if (!mHiddenEthChainIds.contains(network.chainId)) {
                    mHiddenEthChainIds.add(network.chainId);
                }
            }
            case CoinType.FIL -> {
                if (!mHiddenFilChainIds.contains(network.chainId)) {
                    mHiddenFilChainIds.add(network.chainId);
                }
            }
            case CoinType.SOL -> {
                if (!mHiddenSolChainIds.contains(network.chainId)) {
                    mHiddenSolChainIds.add(network.chainId);
                }
            }
            case CoinType.BTC -> {
                if (!mHiddenBtcChainIds.contains(network.chainId)) {
                    mHiddenBtcChainIds.add(network.chainId);
                }
            }
            case CoinType.ZEC -> Log.e(
                    TAG, "Unsupported hidden chain for coin type: " + network.coin);
        }
    }

    private void removeHiddenNetwork(@NonNull final NetworkInfo network) {
        switch (network.coin) {
            case CoinType.ETH -> mHiddenEthChainIds.remove(network.chainId);
            case CoinType.FIL -> mHiddenFilChainIds.remove(network.chainId);
            case CoinType.SOL -> mHiddenSolChainIds.remove(network.chainId);
            case CoinType.BTC -> mHiddenBtcChainIds.remove(network.chainId);
            case CoinType.ZEC -> Log.e(
                    TAG, "Unsupported hidden chain for coin type: " + network.coin);
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

    public static class NetworkListContainer {
        @NonNull final List<NetworkInfo> mNetworks;
        @NonNull final String mActiveChainId;
        @NonNull final List<String> mHiddenChainIds;
        @NonNull final List<String> mCustomChainIds;

        public NetworkListContainer(
                @NonNull final NetworkInfo[] networks,
                @NonNull final String activeChainId,
                @NonNull final String[] hiddenChainIds,
                @Nullable final String[] customChainIds) {
            mNetworks = new ArrayList<>(List.of(networks));
            mActiveChainId = activeChainId;
            mHiddenChainIds = new ArrayList<>(List.of(hiddenChainIds));
            mCustomChainIds =
                    customChainIds != null
                            ? new ArrayList<>(List.of(customChainIds))
                            : new ArrayList<>();
        }
    }
}
