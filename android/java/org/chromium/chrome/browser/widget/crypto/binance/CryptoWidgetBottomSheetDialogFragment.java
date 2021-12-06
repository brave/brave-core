/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.widget.crypto.binance;

import android.content.DialogInterface;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.FragmentPagerAdapter;
import androidx.viewpager.widget.ViewPager;

import com.google.android.material.bottomsheet.BottomSheetDialogFragment;
import com.google.android.material.tabs.TabLayout;

import org.json.JSONException;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.widget.crypto.binance.BinanceAccountBalance;
import org.chromium.chrome.browser.widget.crypto.binance.BinanceCoinNetworks;
import org.chromium.chrome.browser.widget.crypto.binance.BinanceNativeWorker;
import org.chromium.chrome.browser.widget.crypto.binance.BinanceObserver;
import org.chromium.chrome.browser.widget.crypto.binance.BinanceWidgetManager;

import java.util.ArrayList;
import java.util.List;

public class CryptoWidgetBottomSheetDialogFragment extends BottomSheetDialogFragment{
    final public static String TAG_FRAGMENT = "CRYPTO_WIDGET_FRAG";
    private ViewPager viewPager;
    private CryptoWidgetTabAdapter adapter;
    private CryptoWidgetBottomSheetDialogDismissListener
            mCryptoWidgetBottomSheetDialogDismissListener;

    public interface BinanceBottomSheetListener {
        void onContinue();
        void dismissBottomSheetDialog();
    }

    public interface CryptoWidgetBottomSheetDialogDismissListener {
        void onCryptoWidgetBottomSheetDialogDismiss();
    }

    public void setCryptoWidgetBottomSheetDialogDismissListener(
            CryptoWidgetBottomSheetDialogDismissListener
                    cryptoWidgetBottomSheetDialogDismissListener) {
        this.mCryptoWidgetBottomSheetDialogDismissListener =
                cryptoWidgetBottomSheetDialogDismissListener;
    }

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        View view = inflater.inflate(
                R.layout.fragment_crypto_widget_bottom_sheet_dialog, container, false);

        return view;
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        viewPager = view.findViewById(R.id.viewpager);
        TabLayout tabLayout = view.findViewById(R.id.tablayout);

        assert getFragmentManager() != null;
        adapter = new CryptoWidgetTabAdapter(getChildFragmentManager(),
                FragmentPagerAdapter.BEHAVIOR_RESUME_ONLY_CURRENT_FRAGMENT);
        adapter.setBinanceBottomSheetListener(binanceBottomSheetListener);
        viewPager.setAdapter(adapter);
        tabLayout.setupWithViewPager(viewPager);
    }

    @Override
    public void onDismiss(DialogInterface dialog) {
        super.onDismiss(dialog);
        mCryptoWidgetBottomSheetDialogDismissListener.onCryptoWidgetBottomSheetDialogDismiss();
    }

    private BinanceBottomSheetListener binanceBottomSheetListener =
            new BinanceBottomSheetListener() {
                @Override
                public void onContinue() {
                    if (viewPager != null && adapter != null) viewPager.setAdapter(adapter); 
                }

                @Override
                public void dismissBottomSheetDialog() {
                    dismiss();
                }
            };
}