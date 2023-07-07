/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentTransaction;

import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.domain.WalletModel;
import org.chromium.chrome.browser.crypto_wallet.activities.BuySendSwapActivity;
import org.chromium.chrome.browser.crypto_wallet.util.JavaUtils;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;

public class SwapBottomSheetDialogFragment
        extends WalletBottomSheetDialogFragment implements View.OnClickListener {
    public static final String TAG_FRAGMENT = SwapBottomSheetDialogFragment.class.getName();
    private static final String TAG = "BSS-bottom-dialog";
    private LinearLayout mBuyLayout;
    private LinearLayout mSendLayout;
    private LinearLayout mSwapLayout;
    private NetworkInfo mNetworkInfo;
    private WalletModel mWalletModel;

    public static SwapBottomSheetDialogFragment newInstance() {
        return new SwapBottomSheetDialogFragment();
    }

    public void setNetwork(NetworkInfo networkInfo) {
        mNetworkInfo = networkInfo;
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

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        try {
            mWalletModel = BraveActivity.getBraveActivity().getWalletModel();
            registerKeyringObserver(mWalletModel.getKeyringModel());
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "onCreate ", e);
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
    public void onClick(View view) {
        BuySendSwapActivity.ActivityType activityType = BuySendSwapActivity.ActivityType.BUY;
        if (view == mSendLayout) {
            activityType = BuySendSwapActivity.ActivityType.SEND;
        } else if (view == mSwapLayout) {
            activityType = BuySendSwapActivity.ActivityType.SWAP_V2;
        }

        if (!JavaUtils.anyNull(mWalletModel, mNetworkInfo)) {
            BuySendSwapActivity.ActivityType finalActivityType = activityType;
            mWalletModel.getNetworkModel().setNetwork(
                    mNetworkInfo, isSelected -> { openBssAndDismiss(finalActivityType); });
            return;
        }
        openBssAndDismiss(activityType);
    }

    // Open buy send swap (BSS)
    private void openBssAndDismiss(BuySendSwapActivity.ActivityType activityType) {
        Utils.openBuySendSwapActivity(getActivity(), activityType, null);
        dismiss();
    }
}
