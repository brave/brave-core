/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments;

import android.annotation.SuppressLint;
import android.app.Dialog;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewParent;
import android.widget.LinearLayout;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentTransaction;

import com.google.android.material.bottomsheet.BottomSheetDialogFragment;

import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.activities.BuySendSwapActivity;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.crypto_wallet.util.WalletConstants;

public class SwapBottomSheetDialogFragment
        extends BottomSheetDialogFragment implements View.OnClickListener {
    public static final String TAG_FRAGMENT = SwapBottomSheetDialogFragment.class.getName();
    private LinearLayout mBuyLayout;
    private LinearLayout mSendLayout;
    private LinearLayout mSwapLayout;
    private NetworkInfo mNetworkInfo;
    private boolean mIsSwapSupported;
    private boolean mIsCustomNetwork;

    public SwapBottomSheetDialogFragment(boolean isSwapSupported) {
        mIsSwapSupported = isSwapSupported;
    }

    public static SwapBottomSheetDialogFragment newInstance(boolean isSwapSupported) {
        return new SwapBottomSheetDialogFragment(isSwapSupported);
    }

    public void setIsCustomNetwork(boolean isCustomNetwork) {
        mIsCustomNetwork = isCustomNetwork;
    }

    public void setNetwork(NetworkInfo networkInfo) {
        mNetworkInfo = networkInfo;
        updateViewState();
    }

    @Override
    public void show(FragmentManager manager, String tag) {
        try {
            SwapBottomSheetDialogFragment fragment =
                    (SwapBottomSheetDialogFragment) manager.findFragmentByTag(
                            SwapBottomSheetDialogFragment.TAG_FRAGMENT);
            FragmentTransaction transaction = manager.beginTransaction();
            if (fragment != null) {
                transaction.remove(fragment);
            }
            transaction.add(this, tag);
            transaction.commitAllowingStateLoss();
        } catch (IllegalStateException e) {
            Log.e("SwapBottomSheetDialogFragment", e.getMessage());
        }
    }

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        final View view =
                LayoutInflater.from(getContext()).inflate(R.layout.swap_bottom_sheet, null);

        mBuyLayout = view.findViewById(R.id.buy_layout);
        mBuyLayout.setOnClickListener(this);
        mSendLayout = view.findViewById(R.id.send_layout);
        mSendLayout.setOnClickListener(this);
        mSwapLayout = view.findViewById(R.id.swap_layout);
        mSwapLayout.setOnClickListener(this);
        return view;
    }

    @Override
    public void onStart() {
        super.onStart();
        updateViewState();
    }

    private void updateViewState() {
        if (getView() == null) return;
        if (!mIsCustomNetwork && mNetworkInfo.coin == CoinType.ETH
                && Utils.allowSwap(mNetworkInfo.chainId)) {
            mSwapLayout.setVisibility(View.VISIBLE);
        } else {
            mSwapLayout.setVisibility(View.GONE);
        }

        if (!Utils.allowBuy(mNetworkInfo.chainId)) {
            mBuyLayout.setVisibility(View.GONE);
        }
    }

    @Override
    public void onClick(View view) {
        BuySendSwapActivity.ActivityType activityType = BuySendSwapActivity.ActivityType.BUY;
        if (view == mSendLayout) {
            activityType = BuySendSwapActivity.ActivityType.SEND;
        } else if (view == mSwapLayout) {
            activityType = BuySendSwapActivity.ActivityType.SWAP;
        }
        Utils.openBuySendSwapActivity(getActivity(), activityType);
        dismiss();
    }
}
