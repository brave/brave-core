/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import static org.chromium.components.browser_ui.widget.listmenu.BasicListMenu.buildMenuListItem;

import android.annotation.SuppressLint;
import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.core.view.ViewCompat;
import androidx.recyclerview.widget.RecyclerView;
import androidx.recyclerview.widget.RecyclerView.ViewHolder;

import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.chrome.R;
import org.chromium.components.browser_ui.widget.listmenu.BasicListMenu;
import org.chromium.components.browser_ui.widget.listmenu.ListMenu;
import org.chromium.components.browser_ui.widget.listmenu.ListMenuButton;
import org.chromium.components.browser_ui.widget.listmenu.ListMenuButtonDelegate;
import org.chromium.components.browser_ui.widget.listmenu.ListMenuItemProperties;
import org.chromium.ui.modelutil.MVCListAdapter.ModelList;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * BaseAdapter for {@link RecyclerView}. It manages networks to list there.
 */
public class NetworkListBaseAdapter extends RecyclerView.Adapter<ViewHolder> {
    interface ItemClickListener {
        void onItemClicked(NetworkInfo chain, boolean activeNetwork);
        void onItemRemove(NetworkInfo chain);
        void onItemSetAsActive(NetworkInfo chain);
    }

    protected List<NetworkInfo> mElements;
    protected String mActiveChainId;
    protected final Context mContext;
    private ItemClickListener mListener;

    static class RowViewHolder extends ViewHolder {
        private TextView mTitle;
        private TextView mDescription;
        private LinearLayout mItem;
        private ItemClickListener mListener;

        private ListMenuButton mMoreButton;

        RowViewHolder(View view, ItemClickListener listener) {
            super(view);

            mTitle = view.findViewById(R.id.title);
            mDescription = view.findViewById(R.id.description);

            mMoreButton = view.findViewById(R.id.more);
            mItem = view.findViewById(R.id.network_item);
            mListener = listener;
        }

        void updateNetworkInfo(NetworkInfo item, boolean activeNetwork) {
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
            mItem.setOnClickListener(view -> mListener.onItemClicked(item, activeNetwork));
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

    NetworkListBaseAdapter(Context context, ItemClickListener listener) {
        mContext = context;
        mListener = listener;
    }

    @Override
    public ViewHolder onCreateViewHolder(ViewGroup viewGroup, int i) {
        View row = LayoutInflater.from(viewGroup.getContext())
                           .inflate(R.layout.brave_wallet_network_item, viewGroup, false);
        return new RowViewHolder(row, mListener);
    }

    @Override
    @SuppressLint("NotifyDataSetChanged")
    public void onBindViewHolder(ViewHolder viewHolder, int i) {
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

        ListMenu.Delegate delegate = (model) -> {
            int textId = model.get(ListMenuItemProperties.TITLE_ID);
            if (textId == R.string.edit) {
                mListener.onItemClicked(info, activeNetwork);
            } else if (textId == R.string.remove) {
                mListener.onItemRemove(info);
            } else if (textId == R.string.brave_wallet_add_network_set_as_active) {
                mListener.onItemSetAsActive(info);
            }
        };
        ((RowViewHolder) viewHolder).setMenuButtonDelegate(() -> {
            return new BasicListMenu(mContext, menuItems, delegate);
        });
    }

    @Override
    public int getItemCount() {
        return mElements.size();
    }

    /**
     * Sets the displayed networks
     */
    @SuppressLint("NotifyDataSetChanged")
    void setDisplayedNetworks(String activeChainId, NetworkInfo[] networks) {
        mActiveChainId = activeChainId;
        mElements = new ArrayList<>(Arrays.asList(networks));
        notifyDataSetChanged();
    }
}
