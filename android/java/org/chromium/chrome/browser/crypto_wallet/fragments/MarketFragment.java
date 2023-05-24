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
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.widget.AppCompatImageView;
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
import org.chromium.chrome.browser.app.domain.MarketModel.CoinMarketsCallback;
import org.chromium.chrome.browser.app.domain.WalletModel;
import org.chromium.chrome.browser.app.helpers.Api33AndPlusBackPressHelper;
import org.chromium.chrome.browser.crypto_wallet.activities.AccountDetailActivity;
import org.chromium.chrome.browser.crypto_wallet.activities.AddAccountActivity;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletActivity;
import org.chromium.chrome.browser.crypto_wallet.adapters.MarketCoinAdapter;
import org.chromium.chrome.browser.crypto_wallet.adapters.WalletCoinAdapter;
import org.chromium.chrome.browser.crypto_wallet.listeners.OnWalletListItemClick;
import org.chromium.chrome.browser.crypto_wallet.model.WalletListItemModel;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.settings.BraveWalletPreferences;
import org.chromium.chrome.browser.settings.SettingsLauncherImpl;
import org.chromium.components.browser_ui.settings.SettingsLauncher;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class MarketFragment extends Fragment implements CoinMarketsCallback {
    private static final String TAG = "MarketFragment";

    private View rootView;
    private RecyclerView mCoinMarkets;
    private MarketCoinAdapter mMarketCoinAdapter;
    private WalletModel mWalletModel;
    private MarketModel mMarketModel;

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
        rootView = inflater.inflate(R.layout.fragment_market, container, false);
        mCoinMarkets = rootView.findViewById(R.id.coin_markets_recycler_view);
        mCoinMarkets.addItemDecoration(
                new DividerItemDecoration(requireContext(), DividerItemDecoration.VERTICAL));
        mCoinMarkets.setAdapter(mMarketCoinAdapter);
        mCoinMarkets.setLayoutManager(new LinearLayoutManager(getActivity()));
        mMarketModel.getCoinMarkets(this);
        return rootView;
    }

    @Override
    public void onCoinMarketsSuccess(final CoinMarket[] coinMarkets) {
        mMarketCoinAdapter.setCoinMarkets(Arrays.asList(coinMarkets));
    }

    @Override
    public void onCoinMarketsFail() {
        // No-op.
    }
}
