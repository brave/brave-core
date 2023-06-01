/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments;

import android.app.Activity;
import android.content.Intent;
import android.os.Build;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.widget.AppCompatImageView;
import androidx.cardview.widget.CardView;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentActivity;
import androidx.recyclerview.widget.DividerItemDecoration;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.google.android.material.bottomsheet.BottomSheetDialogFragment;

import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.BraveWalletConstants;
import org.chromium.brave_wallet.mojom.CoinMarket;
import org.chromium.brave_wallet.mojom.KeyringInfo;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.domain.MarketModel;
import org.chromium.chrome.browser.app.domain.WalletModel;
import org.chromium.chrome.browser.app.helpers.Api33AndPlusBackPressHelper;
import org.chromium.chrome.browser.app.shimmer.ShimmerFrameLayout;
import org.chromium.chrome.browser.crypto_wallet.activities.AccountDetailActivity;
import org.chromium.chrome.browser.crypto_wallet.activities.AddAccountActivity;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletActivity;
import org.chromium.chrome.browser.crypto_wallet.adapters.MarketCoinAdapter;
import org.chromium.chrome.browser.crypto_wallet.adapters.WalletCoinAdapter;
import org.chromium.chrome.browser.crypto_wallet.listeners.OnWalletListItemClick;
import org.chromium.chrome.browser.crypto_wallet.model.WalletListItemModel;
import org.chromium.chrome.browser.crypto_wallet.util.AndroidUtils;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.custom_layout.LockableNestedScrollView;
import org.chromium.chrome.browser.settings.BraveWalletPreferences;
import org.chromium.chrome.browser.settings.SettingsLauncherImpl;
import org.chromium.components.browser_ui.settings.SettingsLauncher;

import java.util.Arrays;

public class MarketFragment extends Fragment {
    private static final String TAG = "MarketFragment";

    private LockableNestedScrollView mRootView;
    private RecyclerView mCoinMarkets;
    private ShimmerFrameLayout mShimmerViewContainer;
    private LinearLayout mSingleItemShimmer;
    private CardView mShimmerCardView;
    private LinearLayout mShimmerListContainer;
    private CardView mMarketListCardView;

    private MarketCoinAdapter mMarketCoinAdapter;

    private WalletModel mWalletModel;
    private MarketModel mMarketModel;

    private boolean mShimmerActive = true;

    public static MarketFragment newInstance() {
        return new MarketFragment();
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setHasOptionsMenu(true);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            Api33AndPlusBackPressHelper.create(
                    this, (FragmentActivity) requireActivity(), () -> requireActivity().finish());
        }
        try {
            BraveActivity activity = BraveActivity.getBraveActivity();
            mWalletModel = activity.getWalletModel();
            mMarketModel = mWalletModel.getMarketModel();
        } catch (BraveActivity.BraveActivityNotFoundException exception) {
            Log.e(TAG, "onCreate", exception);
        }
        mMarketCoinAdapter = new MarketCoinAdapter(getContext(), mMarketModel);
    }

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        mRootView = (LockableNestedScrollView) inflater.inflate(
                R.layout.fragment_market, container, false);
        mRootView.setScrollingEnabled(false);
        mShimmerViewContainer = mRootView.findViewById(R.id.shimmer_view_container);
        mSingleItemShimmer = mRootView.findViewById(R.id.market_coin_item);
        mShimmerCardView = mRootView.findViewById(R.id.card_view_market_placeholder);
        mShimmerListContainer = mRootView.findViewById(R.id.shimmer_list);
        mMarketListCardView = mRootView.findViewById(R.id.market_list_card_view_view_container);

        mCoinMarkets = mRootView.findViewById(R.id.coin_markets_recycler_view);
        mCoinMarkets.addItemDecoration(
                new DividerItemDecoration(requireContext(), DividerItemDecoration.VERTICAL));
        mCoinMarkets.setAdapter(mMarketCoinAdapter);
        mCoinMarkets.setLayoutManager(new LinearLayoutManager(getActivity()));

        final int visibleShimmerItems =
                getVisibleShimmerItems(mSingleItemShimmer, mShimmerCardView);
        for (int i = 1; i < visibleShimmerItems; i++) {
            inflater.inflate(R.layout.brave_wallet_divider, mShimmerListContainer, true);
            inflater.inflate(R.layout.market_coin_shimmer_item, mShimmerListContainer, true);
        }

        mMarketModel.mCoinMarkets.observe(getViewLifecycleOwner(), coinMarkets -> {
            if (coinMarkets == null) {
                Log.e(TAG, "Failed to load coin market list.");
            } else {
                mMarketCoinAdapter.setCoinMarkets(Arrays.asList(coinMarkets));
            }
            disableShimmerEffect();
        });
        mMarketModel.getCoinMarkets();

        return mRootView;
    }

    private int getVisibleShimmerItems(
            final ViewGroup mSingleItemShimmer, final CardView shimmerCardView) {
        int widthMeasureSpec = View.MeasureSpec.makeMeasureSpec(0, View.MeasureSpec.UNSPECIFIED);
        int heightMeasureSpec = View.MeasureSpec.makeMeasureSpec(0, View.MeasureSpec.UNSPECIFIED);

        mSingleItemShimmer.measure(widthMeasureSpec, heightMeasureSpec);
        int itemHeight = mSingleItemShimmer.getMeasuredHeight();
        int totalHeight = AndroidUtils.getScreenHeight();

        int totalHeightNoPadding =
                totalHeight - shimmerCardView.getPaddingTop() - shimmerCardView.getPaddingBottom();

        return Math.max(1, (int) Math.ceil((float) totalHeightNoPadding / itemHeight));
    }

    @Override
    public void onResume() {
        super.onResume();
        if (mShimmerActive) {
            mShimmerViewContainer.startShimmer();
        }
    }

    @Override
    public void onPause() {
        super.onPause();
        if (mShimmerActive) {
            mShimmerViewContainer.stopShimmer();
        }
    }

    private void disableShimmerEffect() {
        if (mShimmerActive) {
            mShimmerViewContainer.hideShimmer();
            mShimmerViewContainer.setVisibility(View.GONE);
            mRootView.setScrollingEnabled(true);
            mMarketListCardView.setVisibility(View.VISIBLE);
            mShimmerActive = false;
        }
    }
}
