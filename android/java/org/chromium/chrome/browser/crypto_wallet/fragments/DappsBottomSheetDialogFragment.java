/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.Dialog;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewParent;
import android.widget.Button;
import android.widget.LinearLayout;

import androidx.annotation.NonNull;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentTransaction;

import com.google.android.material.bottomsheet.BottomSheetDialogFragment;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.crypto_wallet.activities.BuySendSwapActivity;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;

public class DappsBottomSheetDialogFragment
        extends BottomSheetDialogFragment implements View.OnClickListener {
    public static final String TAG_FRAGMENT = DappsBottomSheetDialogFragment.class.getName();

    private View mMainView;
    private Button mbtUnlock;
    private boolean mShowOnboarding;

    public static DappsBottomSheetDialogFragment newInstance() {
        return new DappsBottomSheetDialogFragment();
    }

    public void showOnboarding(boolean showOnboarding) {
        mShowOnboarding = showOnboarding;
    }

    @Override
    public void show(FragmentManager manager, String tag) {
        try {
            DappsBottomSheetDialogFragment fragment =
                    (DappsBottomSheetDialogFragment) manager.findFragmentByTag(
                            DappsBottomSheetDialogFragment.TAG_FRAGMENT);
            FragmentTransaction transaction = manager.beginTransaction();
            if (fragment != null) {
                transaction.remove(fragment);
            }
            transaction.add(this, tag);
            transaction.commitAllowingStateLoss();
        } catch (IllegalStateException e) {
            Log.e("DappsBottomSheetDialogFragment", e.getMessage());
        }
    }

    @Override
    public void setupDialog(@NonNull Dialog dialog, int style) {
        super.setupDialog(dialog, style);

        @SuppressLint("InflateParams")
        final View view =
                LayoutInflater.from(getContext()).inflate(R.layout.dapps_bottom_sheet, null);

        mMainView = view;
        dialog.setContentView(mMainView);
        ViewParent parent = mMainView.getParent();
        ((View) parent).getLayoutParams().height = ViewGroup.LayoutParams.WRAP_CONTENT;
        mbtUnlock = mMainView.findViewById(R.id.unlock);
        assert mbtUnlock != null;
        if (!mShowOnboarding) {
            mbtUnlock.setText(getResources().getString(R.string.unlock));
        } else {
            mbtUnlock.setText(getResources().getString(R.string.setup_crypto));
        }
        mbtUnlock.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (!mShowOnboarding) {
                    // TODO show unlock
                } else {
                    BraveActivity activity = BraveActivity.getBraveActivity();
                    assert activity != null;
                    if (activity == null) {
                        return;
                    }
                    activity.openBraveWallet();
                }
                dismiss();
            }
        });
    }

    @Override
    public void onClick(View view) {
        // BuySendSwapActivity.ActivityType activityType = BuySendSwapActivity.ActivityType.BUY;
        // if (view == mSendLayout) {
        //     activityType = BuySendSwapActivity.ActivityType.SEND;
        // } else if (view == mSwapLayout) {
        //     activityType = BuySendSwapActivity.ActivityType.SWAP;
        // }
        // Utils.openBuySendSwapActivity(getActivity(), activityType);
        // dismiss();
    }
}
