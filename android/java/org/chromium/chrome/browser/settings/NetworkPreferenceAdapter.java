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
import androidx.recyclerview.widget.RecyclerView;
import androidx.recyclerview.widget.RecyclerView.ViewHolder;

import org.chromium.base.Callback;
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
    private final ItemClickListener mListener;
    private final Context mContext;
    private final List<NetworkInfo> mElements;
    private String mActiveEthChainId;
    private final List<String> mCustomChainIds;
    private final List<String> mHiddenChainIds;

    /**
     * Listener implemented by {@link BraveWalletNetworksPreference} used to handle network
     * operations.
     */
    interface ItemClickListener {
        /** Triggered when a specific network is clicked to be modified. */
        void onItemEdit(@NonNull final NetworkInfo chain, final boolean activeNetwork);

        /** Triggered to remove a custom network completely. */
        void onItemRemove(@NonNull final NetworkInfo chain, @NonNull final Callback<Boolean> callback);

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
            @NonNull final String defaultChainId,
            @NonNull final NetworkInfo[] networks,
            @NonNull final String[] customChainIds,
            @NonNull final String[] hiddenChainIds,
            @NonNull final ItemClickListener listener) {
        mContext = context;
        mActiveEthChainId = defaultChainId;
        mCustomChainIds = new ArrayList<>(List.of(customChainIds));
        mHiddenChainIds = new ArrayList<>(List.of(hiddenChainIds));
        mElements = getNetworkInfoByChainId(mCustomChainIds, networks);
        mListener = listener;
    }

    @NonNull
    private List<NetworkInfo> getNetworkInfoByChainId(
            @NonNull final List<String> customChainIds, @NonNull final NetworkInfo[] allNetworks) {
        final List<NetworkInfo> customChains = new ArrayList<>();
        final List<NetworkInfo> defaultChains = new ArrayList<>();
        final List<NetworkInfo> result = new ArrayList<>();
        for (NetworkInfo network : allNetworks) {
            if (customChainIds.contains(network.chainId)) {
                customChains.add(network);
            } else {
                defaultChains.add(network);
            }
        }

        result.addAll(defaultChains);
        result.addAll(customChains);
        return result;
    }

    @NonNull
    @Override
    public ViewHolder onCreateViewHolder(@NonNull ViewGroup viewGroup, int viewType) {
        View row =
                LayoutInflater.from(viewGroup.getContext())
                        .inflate(R.layout.brave_wallet_network_item, viewGroup, false);
        return new RowViewHolder(row);
    }

    @Override
    public void onBindViewHolder(@NonNull ViewHolder viewHolder, int position) {

        final NetworkInfo info = mElements.get(position);
        final boolean activeNetwork = info.chainId.equals(mActiveEthChainId);
        final boolean customNetwork = mCustomChainIds.contains(info.chainId);
        final RowViewHolder rowViewHolder = (RowViewHolder) viewHolder;
        final boolean visible;

        rowViewHolder.mTitle.setText(info.chainName);
        if (activeNetwork) {
            rowViewHolder.mTitle.setTypeface(null, Typeface.BOLD);
            rowViewHolder.mShowHideNetwork.setEnabled(false);
        } else {
            rowViewHolder.mTitle.setTypeface(null);
            rowViewHolder.mShowHideNetwork.setEnabled(true);
        }

        if (mHiddenChainIds.contains(info.chainId) && !activeNetwork) {
            visible = false;
            rowViewHolder.mShowHideNetwork.setImageResource(R.drawable.ic_eye_off);
        } else {
            visible = true;
            rowViewHolder.mShowHideNetwork.setImageResource(R.drawable.ic_eye_on);
        }

        StringBuilder description = new StringBuilder(info.chainId);
        if (info.activeRpcEndpointIndex >= 0
                && info.activeRpcEndpointIndex < info.rpcEndpoints.length) {
            description.append(" ").append(info.rpcEndpoints[info.activeRpcEndpointIndex].url);
        }
        rowViewHolder.mDescription.setText(description.toString());

        rowViewHolder.mMoreButton.setContentDescriptionContext(info.chainName);

        // The more button will become visible if setMenuButtonDelegate is called.
        rowViewHolder.mMoreButton.setVisibility(View.INVISIBLE);

        // When a network is set as active, it cannot be removed or edited.
        if (activeNetwork) {
            return;
        }

        rowViewHolder.mShowHideNetwork.setOnClickListener(
                v ->
                        mListener.onItemVisibilityChange(
                                info,
                                !visible,
                                result -> {
                                    if (result) {
                                        if (visible) {
                                            if (!mHiddenChainIds.contains(info.chainId)) {
                                                mHiddenChainIds.add(info.chainId);
                                            }
                                            rowViewHolder.mShowHideNetwork.setImageResource(
                                                    R.drawable.ic_eye_off);
                                        } else {
                                            mHiddenChainIds.remove(info.chainId);
                                            rowViewHolder.mShowHideNetwork.setImageResource(
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
                        mListener.onItemEdit(info, false);
                    } else if (textId == R.string.remove) {
                        mListener.onItemRemove(info, result -> {
                            if (result) {
                                mCustomChainIds.remove(info.chainId);
                                // The network may or may not be hidden.
                                mHiddenChainIds.remove(info.chainId);
                                mElements.remove(info);
                                notifyItemRemoved(position);
                            }
                        });
                    } else if (textId == R.string.brave_wallet_add_network_set_as_active) {
                        mListener.onItemSetAsActive(
                                info,
                                result -> {
                                    if (result) {
                                        final String oldActiveEthChainId = mActiveEthChainId;
                                        mActiveEthChainId = info.chainId;

                                        final int oldActiveEthChainIdIndex =
                                                findChainIdPosition(oldActiveEthChainId);
                                        final int newActiveEthChainIdIndex =
                                                findChainIdPosition(mActiveEthChainId);

                                        if (oldActiveEthChainIdIndex != NO_POSITION) {
                                            notifyItemChanged(oldActiveEthChainIdIndex);
                                        }

                                        if (newActiveEthChainIdIndex != NO_POSITION) {
                                            notifyItemChanged(newActiveEthChainIdIndex);
                                        }
                                    }
                                });
                    }
                };
        rowViewHolder.setMenuButtonDelegate(
                () -> {
                    View contentView =
                            LayoutInflater.from(mContext)
                                    .inflate(R.layout.app_menu_layout, rowViewHolder.mItem, false);
                    ListView listView = contentView.findViewById(R.id.app_menu_list);
                    return new BasicListMenu(
                            mContext, menuItems, contentView, listView, delegate, 0);
                });
    }

    private int findChainIdPosition(@NonNull final String chainId) {
        for (int i = 0; i < mElements.size(); i++) {
            if (chainId.equals(mElements.get(i).chainId)) {
                return i;
            }
        }
        return NO_POSITION;
    }

    @Override
    public int getItemCount() {
        return mElements.size();
    }

    static class RowViewHolder extends ViewHolder {
        private final TextView mTitle;
        private final TextView mDescription;
        private final LinearLayout mItem;
        private final ImageView mShowHideNetwork;

        private final ListMenuButton mMoreButton;

        RowViewHolder(@NonNull final View view) {
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
}
