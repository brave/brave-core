/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.adapters;

import static androidx.recyclerview.widget.RecyclerView.NO_POSITION;

import android.annotation.SuppressLint;
import android.content.Context;
import android.graphics.ColorMatrix;
import android.graphics.ColorMatrixColorFilter;
import android.os.Handler;
import android.os.Looper;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.DrawableRes;
import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import com.google.android.material.checkbox.MaterialCheckBox;

import org.chromium.brave_wallet.mojom.BraveWalletConstants;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.domain.NetworkModel;
import org.chromium.chrome.browser.crypto_wallet.fragments.onboarding.OnboardingNetworkSelectionFragment;
import org.chromium.chrome.browser.crypto_wallet.util.NetworkUtils;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Set;
import java.util.stream.Collectors;

/** Grid adapter used during Wallet onboarding flow that shows all available networks. */
public class OnboardingNetworkSelectorGridAdapter
        extends RecyclerView.Adapter<RecyclerView.ViewHolder> {
    /**
     * Network selection listener used to notify how many networks have been selected.
     *
     * @see OnboardingNetworkSelectionFragment
     */
    public interface OnNetworkSelectionListener {
        void selectedNetworks(final int selectedNetworks);
    }

    public static final int NETWORK_ITEM_VIEW_TYPE = 0;
    public static final int LABEL_FEATURED_VIEW_TYPE = 1;
    public static final int LABEL_POPULAR_VIEW_TYPE = 2;

    private static final int PAYLOAD_SELECT_ALL = 1;

    private static final int DEBOUNCE_SEARCH_MILLIS = 500;

    private static final List<String> ALWAYS_SELECTED_CHAIN_IDS =
            Arrays.asList(
                    BraveWalletConstants.MAINNET_CHAIN_ID, BraveWalletConstants.SOLANA_MAINNET);

    private final List<NetworkInfo> mPrimaryNetworks;
    private final List<NetworkInfo> mSecondaryNetworks;
    private final List<NetworkInfo> mTestNetworks;

    private final List<NetworkInfo> mFilteredPrimaryNetworks;
    private final List<NetworkInfo> mFilteredSecondaryNetworks;
    private final List<NetworkInfo> mFilteredTestNetworks;

    private final Set<Integer> mSelectedNetworks;

    @NonNull private final OnNetworkSelectionListener mListener;

    private final String mFeaturedNetworks;
    private final String mPopularNetworks;
    private final String mSelectAll;

    private final Handler mHandler;

    private final Context mContext;
    private boolean mShowTestNetworks;
    private boolean mSearching;
    private String mFilter;

    // Runnable that filters network names.
    private Runnable mFilteringRunnable;
    // Integer that keeps track of the featured header visibility.
    // Shown by default, but it may be hidden when filtered primary network list is empty.
    private int mFeaturedHeaderWhenSearching;
    // Integer that keeps track of the popular header visibility.
    // Shown by default, but it may be hidden if there is not al least one item between
    // filtered networks and filtered secondary networks.
    private int mPopularHeaderWhenSearching;
    // Integer that keeps track of filtered test networks size.
    // When test networks are hidden, it defaults to zero.
    private int mFilteredTestNetworkSize;

    public OnboardingNetworkSelectorGridAdapter(
            @NonNull final Context context,
            final boolean showTestNetworks,
            @NonNull final NetworkModel.NetworkLists availableNetworks,
            @NonNull final OnNetworkSelectionListener listener) {
        mContext = context;

        mShowTestNetworks = showTestNetworks;
        mSearching = false;

        mPrimaryNetworks = availableNetworks.mPrimaryNetworkList;
        mSecondaryNetworks = availableNetworks.mSecondaryNetworkList;
        mTestNetworks = availableNetworks.mTestNetworkList;

        mFilteredPrimaryNetworks = new ArrayList<>();
        mFilteredSecondaryNetworks = new ArrayList<>();
        mFilteredTestNetworks = new ArrayList<>();

        mFeaturedHeaderWhenSearching = 1;
        mFilteredTestNetworkSize = 0;
        mPopularHeaderWhenSearching = 1;

        mListener = listener;

        mSelectedNetworks = new HashSet<>();
        // Pre-select primary networks.
        for (NetworkInfo networkInfo : mPrimaryNetworks) {
            mSelectedNetworks.add(networkInfo.hashCode());
        }
        // Pre-select secondary networks.
        for (NetworkInfo networkInfo : mSecondaryNetworks) {
            mSelectedNetworks.add(networkInfo.hashCode());
        }

        mFeaturedNetworks = context.getString(R.string.wallet_featured);
        mPopularNetworks = context.getString(R.string.wallet_popular);
        mSelectAll = context.getString(R.string.brave_wallet_select_all);

        mHandler = new Handler(Looper.getMainLooper());
    }

    @NonNull
    @Override
    public RecyclerView.ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
        LayoutInflater inflater = LayoutInflater.from(mContext);

        if (viewType == LABEL_FEATURED_VIEW_TYPE) {
            View headerFeaturedViewHolder =
                    inflater.inflate(
                            R.layout.view_holder_onboarding_header_network_selector, parent, false);

            LabelViewHolder labelViewHolder = new LabelViewHolder(headerFeaturedViewHolder);
            labelViewHolder.mLabelName.setText(mFeaturedNetworks);
            labelViewHolder.mSelectAll.setVisibility(View.GONE);
            return labelViewHolder;
        } else if (viewType == LABEL_POPULAR_VIEW_TYPE) {
            View headerPopularViewHolder =
                    inflater.inflate(
                            R.layout.view_holder_onboarding_header_network_selector, parent, false);
            LabelViewHolder labelViewHolder = new LabelViewHolder(headerPopularViewHolder);
            labelViewHolder.mLabelName.setText(mPopularNetworks);
            labelViewHolder.mSelectAll.setText(mSelectAll);
            labelViewHolder.mSelectAll.setVisibility(View.VISIBLE);
            labelViewHolder.mSelectAll.setOnClickListener(
                    view -> {
                        if (mSearching) {
                            for (NetworkInfo networkInfo : mFilteredSecondaryNetworks) {
                                mSelectedNetworks.add(networkInfo.hashCode());
                            }

                            if (mShowTestNetworks) {
                                // Add test networks from filtered test network list.
                                for (NetworkInfo networkInfo : mFilteredTestNetworks) {
                                    mSelectedNetworks.add(networkInfo.hashCode());
                                }
                            }
                            mListener.selectedNetworks(mSelectedNetworks.size());
                            // Add payload to update only part of the UI.
                            notifyItemRangeChanged(
                                    mFilteredPrimaryNetworks.size()
                                            + mFeaturedHeaderWhenSearching
                                            + mPopularHeaderWhenSearching,
                                    mFilteredSecondaryNetworks.size() + mFilteredTestNetworkSize,
                                    PAYLOAD_SELECT_ALL);
                        } else {
                            for (NetworkInfo networkInfo : mSecondaryNetworks) {
                                mSelectedNetworks.add(networkInfo.hashCode());
                            }

                            if (mShowTestNetworks) {
                                // Add test networks from test network list.
                                for (NetworkInfo networkInfo : mTestNetworks) {
                                    mSelectedNetworks.add(networkInfo.hashCode());
                                }
                            }
                            mListener.selectedNetworks(mSelectedNetworks.size());
                            // Add payload to update only part of the UI.
                            notifyItemRangeChanged(
                                    mPrimaryNetworks.size() + 2,
                                    mSecondaryNetworks.size() + mTestNetworks.size(),
                                    PAYLOAD_SELECT_ALL);
                        }
                    });
            return labelViewHolder;
        } else {
            View networkSelectorViewHolder =
                    inflater.inflate(
                            R.layout.view_holder_onboarding_network_selector, parent, false);
            final NetworkViewHolder networkViewHolder =
                    new NetworkViewHolder(networkSelectorViewHolder);
            networkViewHolder.mSelectionCheckbox.setOnCheckedChangeListener(
                    (buttonView, isChecked) -> {
                        final int position = networkViewHolder.getBindingAdapterPosition();
                        if (position != NO_POSITION) {
                            final NetworkInfo networkInfo = getNetworkInfo(position);
                            if (isChecked) {
                                mSelectedNetworks.add(networkInfo.hashCode());
                            } else {
                                mSelectedNetworks.remove(networkInfo.hashCode());
                            }
                            mListener.selectedNetworks(mSelectedNetworks.size());
                        }
                        networkViewHolder.mSelectionCheckbox.setChecked(isChecked);
                        networkViewHolder.itemView.setSelected(isChecked);
                    });
            networkViewHolder.itemView.setOnClickListener(
                    view -> {
                        if (networkViewHolder.mSelectionCheckbox.isEnabled()) {
                            networkViewHolder.mSelectionCheckbox.setChecked(
                                    !networkViewHolder.mSelectionCheckbox.isChecked());
                        }
                    });
            return networkViewHolder;
        }
    }

    @Override
    public void onBindViewHolder(
            @NonNull RecyclerView.ViewHolder holder, int position, @NonNull List<Object> payloads) {
        if (!payloads.isEmpty()) {
            for (final Object payload : payloads) {
                if (payload.equals(PAYLOAD_SELECT_ALL)) {
                    final NetworkViewHolder networkViewHolder = (NetworkViewHolder) holder;
                    if (mSelectedNetworks.contains(getNetworkInfo(position).hashCode())) {
                        networkViewHolder.mSelectionCheckbox.setChecked(true);
                        networkViewHolder.itemView.setSelected(true);
                    }
                }
            }
        } else {
            super.onBindViewHolder(holder, position, payloads);
        }
    }

    @Override
    public void onBindViewHolder(@NonNull RecyclerView.ViewHolder viewHolder, int position) {
        if (viewHolder.getItemViewType() == NETWORK_ITEM_VIEW_TYPE) {
            final NetworkViewHolder networkViewHolder = (NetworkViewHolder) viewHolder;

            NetworkInfo networkInfo = getNetworkInfo(position);
            final boolean enableSelection =
                    !ALWAYS_SELECTED_CHAIN_IDS.contains(networkInfo.chainId);

            networkViewHolder.mSelectionCheckbox.setEnabled(enableSelection);
            networkViewHolder.itemView.setEnabled(enableSelection);

            boolean selected = mSelectedNetworks.contains(networkInfo.hashCode());
            networkViewHolder.mSelectionCheckbox.setChecked(selected);
            networkViewHolder.itemView.setSelected(selected);

            networkViewHolder.mNetworkName.setText(networkInfo.chainName);

            @DrawableRes
            int logo = Utils.getNetworkIconDrawable(networkInfo.chainId, networkInfo.coin);
            if (logo != -1) {
                networkViewHolder.mNetworkLogo.setImageResource(logo);
                if (NetworkUtils.isTestNetwork(networkInfo.chainId)) {
                    // Grey style test net image.
                    ColorMatrix matrix = new ColorMatrix();
                    matrix.setSaturation(0);

                    ColorMatrixColorFilter filter = new ColorMatrixColorFilter(matrix);
                    networkViewHolder.mNetworkLogo.setColorFilter(filter);
                } else {
                    networkViewHolder.mNetworkLogo.setColorFilter(null);
                }
            } else {
                networkViewHolder.mNetworkLogo.setColorFilter(null);
            }
        }
    }

    @SuppressLint("NotifyDataSetChanged")
    public void filter(@NonNull final String text) {
        if (mFilteringRunnable != null) {
            mHandler.removeCallbacks(mFilteringRunnable);
        }

        mFilteringRunnable =
                () -> {
                    mSearching = !text.trim().isEmpty();
                    mFilter = text.trim().toLowerCase(Locale.ENGLISH);

                    mFilteredPrimaryNetworks.clear();
                    mFilteredSecondaryNetworks.clear();
                    mFilteredTestNetworks.clear();

                    if (mSearching) {
                        performFiltering(mPrimaryNetworks, mFilteredPrimaryNetworks, mFilter);
                        performFiltering(mSecondaryNetworks, mFilteredSecondaryNetworks, mFilter);
                        performFiltering(mTestNetworks, mFilteredTestNetworks, mFilter);
                    }

                    // Don't count feature header if filtered primary network list is empty.
                    mFeaturedHeaderWhenSearching = mFilteredPrimaryNetworks.isEmpty() ? 0 : 1;
                    // Calculate filtered test network size only if test networks are shown.
                    mFilteredTestNetworkSize = mShowTestNetworks ? mFilteredTestNetworks.size() : 0;
                    // Popular header label is shown only if there's at least one element.
                    mPopularHeaderWhenSearching =
                            mFilteredSecondaryNetworks.size() + mFilteredTestNetworkSize == 0
                                    ? 0
                                    : 1;
                    notifyDataSetChanged();
                };
        // Debounce filtering to avoid flickering effect on recycler view items.
        mHandler.postDelayed(mFilteringRunnable, DEBOUNCE_SEARCH_MILLIS);
    }

    @SuppressWarnings("NoStreams")
    private void performFiltering(
            @NonNull final List<NetworkInfo> source,
            @NonNull final List<NetworkInfo> filtered,
            @NonNull final String text) {
        //noinspection SimplifyStreamApiCallChains
        filtered.addAll(
                source.stream()
                        .filter(
                                networkInfo ->
                                        networkInfo
                                                .chainName
                                                .toLowerCase(Locale.ENGLISH)
                                                .contains(text))
                        .collect(Collectors.toList()));
    }

    private NetworkInfo getNetworkInfo(int position) {
        if (mSearching) {
            if (position
                    >= mFeaturedHeaderWhenSearching
                            + mFilteredPrimaryNetworks.size()
                            + mFilteredSecondaryNetworks.size()
                            + mPopularHeaderWhenSearching) {
                return mFilteredTestNetworks.get(
                        position
                                - (mFeaturedHeaderWhenSearching
                                        + mPopularHeaderWhenSearching
                                        + mFilteredPrimaryNetworks.size()
                                        + mFilteredSecondaryNetworks.size()));
            } else if (position
                    >= mFeaturedHeaderWhenSearching
                            + mFilteredPrimaryNetworks.size()
                            + mPopularHeaderWhenSearching) {
                return mFilteredSecondaryNetworks.get(
                        position
                                - (mFilteredPrimaryNetworks.size()
                                        + mFeaturedHeaderWhenSearching
                                        + mPopularHeaderWhenSearching));
            } else {
                return mFilteredPrimaryNetworks.get(position - mFeaturedHeaderWhenSearching);
            }
        } else {
            if (position >= mPrimaryNetworks.size() + mSecondaryNetworks.size() + 2) {
                int testPosition = position - (mPrimaryNetworks.size() + mSecondaryNetworks.size());
                return mTestNetworks.get(testPosition - 2);
            } else if (position >= mPrimaryNetworks.size() + 2) {
                int secondaryPosition = position - mPrimaryNetworks.size();
                return mSecondaryNetworks.get(secondaryPosition - 2);
            } else {
                return mPrimaryNetworks.get(position - 1);
            }
        }
    }

    @Override
    public int getItemCount() {
        if (mSearching) {
            return mFeaturedHeaderWhenSearching
                    + mPopularHeaderWhenSearching
                    + mFilteredPrimaryNetworks.size()
                    + mFilteredSecondaryNetworks.size()
                    + mFilteredTestNetworkSize;
        } else {
            final int testNetworkSize = mShowTestNetworks ? mTestNetworks.size() : 0;
            return mPrimaryNetworks.size() + mSecondaryNetworks.size() + testNetworkSize + 2;
        }
    }

    @Override
    public int getItemViewType(int position) {
        if (mSearching) {
            if (position == 0 && !mFilteredPrimaryNetworks.isEmpty()) {
                return LABEL_FEATURED_VIEW_TYPE;
            } else if (position == mFilteredPrimaryNetworks.size() + mFeaturedHeaderWhenSearching
                    && (!mFilteredSecondaryNetworks.isEmpty() || mFilteredTestNetworkSize != 0)) {
                return LABEL_POPULAR_VIEW_TYPE;
            } else {
                return NETWORK_ITEM_VIEW_TYPE;
            }
        } else {
            if (position == 0) {
                return LABEL_FEATURED_VIEW_TYPE;
            } else if (position == mPrimaryNetworks.size() + 1) {
                return LABEL_POPULAR_VIEW_TYPE;
            } else {
                return NETWORK_ITEM_VIEW_TYPE;
            }
        }
    }

    public void showTestNetworks(final boolean show) {
        if (mShowTestNetworks == show) {
            return;
        }
        mShowTestNetworks = show;
        mFilteredTestNetworkSize = mShowTestNetworks ? mFilteredTestNetworks.size() : 0;
        mPopularHeaderWhenSearching =
                mFilteredSecondaryNetworks.size() + mFilteredTestNetworkSize == 0 ? 0 : 1;
        if (mSearching) {
            final int testNetworkSize = mFilteredTestNetworks.size();

            if (!mShowTestNetworks) {
                mFilteredTestNetworks.forEach(
                        networkInfo -> mSelectedNetworks.remove(networkInfo.hashCode()));
                mListener.selectedNetworks(mSelectedNetworks.size());
                int popularHeader = 1;
                // Notify item removed for popular header only if required.
                if (mFilteredSecondaryNetworks.isEmpty() && !mFilteredTestNetworks.isEmpty()) {
                    notifyItemRemoved(
                            mFeaturedHeaderWhenSearching
                                    + mFilteredPrimaryNetworks.size()
                                    + mFilteredSecondaryNetworks.size());
                    // Mark popular header as removed.
                    popularHeader = 0;
                }
                // Notify test networks as removed item and count popular header value that may or
                // may not be present.
                notifyItemRangeRemoved(
                        mFeaturedHeaderWhenSearching
                                + mFilteredPrimaryNetworks.size()
                                + popularHeader
                                + mFilteredSecondaryNetworks.size(),
                        testNetworkSize);
            } else {
                int popularHeader = 0;
                // Notify item inserted for popular header only if required.
                if (mFilteredSecondaryNetworks.isEmpty() && !mFilteredTestNetworks.isEmpty()) {
                    notifyItemInserted(
                            mFeaturedHeaderWhenSearching
                                    + mFilteredPrimaryNetworks.size()
                                    + mFilteredSecondaryNetworks.size());
                    // Mark popular header as inserted.
                    popularHeader = 1;
                }
                // Notify test networks as inserted items and count popular header value that may or
                // may not be present.
                notifyItemRangeInserted(
                        mFeaturedHeaderWhenSearching
                                + mPrimaryNetworks.size()
                                + popularHeader
                                + mSecondaryNetworks.size(),
                        testNetworkSize);
            }
        } else {
            if (!mShowTestNetworks) {
                mTestNetworks.forEach(
                        networkInfo -> mSelectedNetworks.remove(networkInfo.hashCode()));
                mListener.selectedNetworks(mSelectedNetworks.size());
                notifyItemRangeRemoved(
                        mPrimaryNetworks.size() + mSecondaryNetworks.size() + 2,
                        mTestNetworks.size());
            } else {
                notifyItemRangeInserted(
                        mPrimaryNetworks.size() + mSecondaryNetworks.size() + 2,
                        mTestNetworks.size());
            }
        }
    }

    @NonNull
    public Set<NetworkInfo> getSelectedNetworks() {
        final Set<NetworkInfo> selectedNetwork = new HashSet<>();
        selectedNetwork.addAll(extractNetworksFromHashCodes(mPrimaryNetworks));
        selectedNetwork.addAll(extractNetworksFromHashCodes(mSecondaryNetworks));
        selectedNetwork.addAll(extractNetworksFromHashCodes(mTestNetworks));

        return selectedNetwork;
    }

    @NonNull
    private Collection<NetworkInfo> extractNetworksFromHashCodes(
            @NonNull final List<NetworkInfo> networks) {
        final Set<NetworkInfo> extractedNetworks = new HashSet<>();
        for (NetworkInfo networkInfo : networks) {
            if (mSelectedNetworks.contains(networkInfo.hashCode())) {
                extractedNetworks.add(networkInfo);
            }
        }
        return extractedNetworks;
    }

    @NonNull
    public Set<NetworkInfo> getAvailableNetworks() {
        final Set<NetworkInfo> availableNetworks = new HashSet<>();
        availableNetworks.addAll(mPrimaryNetworks);
        availableNetworks.addAll(mSecondaryNetworks);
        availableNetworks.addAll(mTestNetworks);

        return availableNetworks;
    }

    public static class LabelViewHolder extends RecyclerView.ViewHolder {
        final TextView mLabelName;
        final TextView mSelectAll;

        public LabelViewHolder(@NonNull View itemView) {
            super(itemView);
            mLabelName = itemView.findViewById(R.id.network_selector_label_name);
            mSelectAll = itemView.findViewById(R.id.select_all_networks);
        }
    }

    public static class NetworkViewHolder extends RecyclerView.ViewHolder {
        final TextView mNetworkName;
        final ImageView mNetworkLogo;
        final MaterialCheckBox mSelectionCheckbox;

        public NetworkViewHolder(@NonNull View itemView) {
            super(itemView);
            mNetworkName = itemView.findViewById(R.id.network_selector_network_name);
            mNetworkLogo = itemView.findViewById(R.id.network_selector_logo);
            mSelectionCheckbox = itemView.findViewById(R.id.network_selected);
        }
    }
}
