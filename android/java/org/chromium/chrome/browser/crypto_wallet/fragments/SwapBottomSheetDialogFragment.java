/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments;

import android.annotation.SuppressLint;
import android.app.Dialog;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewParent;
import android.widget.LinearLayout;

import androidx.annotation.NonNull;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentTransaction;

import com.google.android.material.bottomsheet.BottomSheetDialogFragment;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.activities.BuySendSwapActivity;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;

public class SwapBottomSheetDialogFragment
        extends BottomSheetDialogFragment implements View.OnClickListener {
    public static final String TAG_FRAGMENT = SwapBottomSheetDialogFragment.class.getName();
    LinearLayout mBuyLayout;
    LinearLayout mSendLayout;
    LinearLayout mSwapLayout;

    public static SwapBottomSheetDialogFragment newInstance() {
        return new SwapBottomSheetDialogFragment();
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
    public void setupDialog(@NonNull Dialog dialog, int style) {
        super.setupDialog(dialog, style);

        @SuppressLint("InflateParams")
        final View view =
                LayoutInflater.from(getContext()).inflate(R.layout.swap_bottom_sheet, null);

        mBuyLayout = view.findViewById(R.id.buy_layout);
        mBuyLayout.setOnClickListener(this);
        mSendLayout = view.findViewById(R.id.send_layout);
        mSendLayout.setOnClickListener(this);
        mSwapLayout = view.findViewById(R.id.swap_layout);
        mSwapLayout.setOnClickListener(this);

        dialog.setContentView(view);
        ViewParent parent = view.getParent();
        ((View) parent).getLayoutParams().height = ViewGroup.LayoutParams.WRAP_CONTENT;
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
