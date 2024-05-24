/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import static org.chromium.components.browser_ui.widget.BrowserUiListMenuUtils.buildMenuListItem;

import android.annotation.SuppressLint;
import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.core.view.ViewCompat;
import androidx.recyclerview.widget.RecyclerView;
import androidx.recyclerview.widget.RecyclerView.ViewHolder;

import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.domain.NetworkModel;
import org.chromium.ui.listmenu.BasicListMenu;
import org.chromium.ui.listmenu.ListMenu;
import org.chromium.ui.listmenu.ListMenuButton;
import org.chromium.ui.listmenu.ListMenuButtonDelegate;
import org.chromium.ui.listmenu.ListMenuItemProperties;
import org.chromium.ui.modelutil.MVCListAdapter.ModelList;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * BaseAdapter for {@link RecyclerView}. It manages networks to list there.
 */
public class NetworkPreferenceAdapter extends RecyclerView.Adapter<ViewHolder> {
    private final ItemClickListener mListener;
    private final Context mContext;
    private final List<NetworkInfo> mElements;
    private final String mActiveChainId;


    interface ItemClickListener {
        void onItemClick(@NonNull final NetworkInfo chain, final boolean activeNetwork);
        void onItemRemove(@NonNull final  NetworkInfo chain);
        void onItemSetAsActive(@NonNull final  NetworkInfo chain);
    }

    public NetworkPreferenceAdapter(@NonNull final Context context, @NonNull final String defaultChainId, @NonNull final NetworkInfo[] networks, @NonNull final String[] customChainIds, @NonNull final String[] hiddenChainIds, @NonNull final ItemClickListener listener) {
        mContext = context;
        mActiveChainId = defaultChainId;
        mElements = getNetworkInfoByChainId(customChainIds, networks);
        mListener = listener;
    }

    @NonNull
    private List<NetworkInfo> getNetworkInfoByChainId(
            @NonNull final String[] chainId, @NonNull final NetworkInfo[] allNetworks) {
        final List<NetworkInfo> result = new ArrayList<>();
        for (NetworkInfo network : allNetworks) {
            if (Arrays.asList(chainId).contains(network.chainId)) {
                result.add(network);
            }
        }

        return result;
    }

    @NonNull
    @Override
    public ViewHolder onCreateViewHolder(@NonNull ViewGroup viewGroup, int i) {
        View row = LayoutInflater.from(viewGroup.getContext())
                           .inflate(R.layout.brave_wallet_network_item, viewGroup, false);
        return new RowViewHolder(row, mListener);
    }

    @Override
    @SuppressLint("NotifyDataSetChanged")
    public void onBindViewHolder(@NonNull ViewHolder viewHolder, int i) {
        final NetworkInfo info = mElements.get(i);
        boolean activeNetwork = info.chainId.equals(mActiveChainId);
        ((RowViewHolder) viewHolder).updateNetworkInfo(info, activeNetwork);

        if (activeNetwork) {
            return;
        }

        ModelList menuItems = new ModelList();
        menuItems.add(buildMenuListItem(R.string.edit, 0, 0));
        menuItems.add(buildMenuListItem(R.string.remove, 0, 0));
        menuItems.add(buildMenuListItem(R.string.brave_wallet_add_network_set_as_active, 0, 0));

        ListMenu.Delegate delegate =
                (model) -> {
                    int textId = model.get(ListMenuItemProperties.TITLE_ID);
                    if (textId == R.string.edit) {
                        mListener.onItemClick(info, false);
                    } else if (textId == R.string.remove) {
                        mListener.onItemRemove(info);
                    } else if (textId == R.string.brave_wallet_add_network_set_as_active) {
                        mListener.onItemSetAsActive(info);
                    }
                };
        ((RowViewHolder) viewHolder)
                .setMenuButtonDelegate(
                        () -> {
                            View contentView =
                                    LayoutInflater.from(mContext)
                                            .inflate(R.layout.app_menu_layout, null);
                            ListView listView = contentView.findViewById(R.id.app_menu_list);
                            return new BasicListMenu(
                                    mContext, menuItems, contentView, listView, delegate, 0);
                        });
    }

    @Override
    public int getItemCount() {
        return mElements.size();
    }

    static class RowViewHolder extends ViewHolder {
        private final TextView mTitle;
        private final TextView mDescription;
        private final LinearLayout mItem;
        private final ItemClickListener mListener;

        private final ListMenuButton mMoreButton;

        RowViewHolder(@NonNull final View view, @NonNull final ItemClickListener listener) {
            super(view);

            mTitle = view.findViewById(R.id.title);
            mDescription = view.findViewById(R.id.description);

            mMoreButton = view.findViewById(R.id.more);
            mItem = view.findViewById(R.id.network_item);
            mListener = listener;
        }

        void updateNetworkInfo(@NonNull final NetworkInfo item, final boolean activeNetwork) {
            mTitle.setText(item.chainName);
            String description = item.chainId;
            if (item.activeRpcEndpointIndex >= 0
                    && item.activeRpcEndpointIndex < item.rpcEndpoints.length) {
                description += " " + item.rpcEndpoints[item.activeRpcEndpointIndex].url;
            }
            mDescription.setText(description);

            mMoreButton.setContentDescriptionContext(item.chainName);

            // The more button will become visible if setMenuButtonDelegate is called.
            mMoreButton.setVisibility(View.GONE);
            mItem.setOnClickListener(view -> mListener.onItemClick(item, activeNetwork));
        }

        /**
         * Sets up the menu button at the end of this row with a given delegate.
         * @param delegate A {@link ListMenuButtonDelegate}.
         */
        void setMenuButtonDelegate(@NonNull ListMenuButtonDelegate delegate) {
            mMoreButton.setVisibility(View.VISIBLE);
            mMoreButton.setDelegate(delegate);
            // Set item row end padding 0 when MenuButton is visible.
            ViewCompat.setPaddingRelative(itemView, ViewCompat.getPaddingStart(itemView),
                    itemView.getPaddingTop(), 0, itemView.getPaddingBottom());
        }
    }
}
