/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.adapters.WalletCoinAdapter;
import org.chromium.chrome.browser.crypto_wallet.util.SmoothLineChartEquallySpaced;

public class CryptoChildFragment extends Fragment {
    public static CryptoChildFragment newInstance() {
        return new CryptoChildFragment();
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setHasOptionsMenu(true);
    }

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_crypto_child, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        assert getActivity() != null;

        SmoothLineChartEquallySpaced chartES = view.findViewById(R.id.line_chart);
        chartES.setData(new float[] {15, 21, 9, 21, 25, 35, 24, 28});

        setUpCoinList(view);
    }

    private void setUpCoinList(View view) {
        RecyclerView rvCoins = view.findViewById(R.id.rvCoins);
        WalletCoinAdapter walletCoinAdapter = new WalletCoinAdapter();
        rvCoins.setAdapter(walletCoinAdapter);
        rvCoins.setLayoutManager(new LinearLayoutManager(getActivity()));
    }
}
