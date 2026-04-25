/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.rate;

import android.content.Context;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentTransaction;

import com.google.android.material.bottomsheet.BottomSheetBehavior;
import com.google.android.material.bottomsheet.BottomSheetDialog;
import com.google.android.material.bottomsheet.BottomSheetDialogFragment;

import org.chromium.base.Log;
import org.chromium.chrome.R;

public class BraveRateDialogFragment extends BottomSheetDialogFragment {
    public static final String TAG_FRAGMENT = "brave_rating_dialog_tag";
    private static final String TAG = "RateDialogFragment";
    private boolean mIsFromSettings;

    public static BraveRateDialogFragment newInstance(boolean isFromSettings) {
        Bundle bundle = new Bundle();
        bundle.putBoolean(RateUtils.FROM_SETTINGS, isFromSettings);

        BraveRateDialogFragment rateDialogFragment = new BraveRateDialogFragment();
        rateDialogFragment.setArguments(bundle);
        return rateDialogFragment;
    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        if (getArguments() != null) {
            mIsFromSettings = getArguments().getBoolean(RateUtils.FROM_SETTINGS);
        }
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setStyle(STYLE_NORMAL, R.style.AppBottomSheetDialogTheme);
    }

    @Override
    public void show(@NonNull FragmentManager manager, @Nullable String tag) {
        try {
            BraveRateDialogFragment fragment = (BraveRateDialogFragment) manager.findFragmentByTag(
                    BraveRateDialogFragment.TAG_FRAGMENT);
            FragmentTransaction transaction = manager.beginTransaction();
            if (fragment != null) {
                transaction.remove(fragment);
            }
            transaction.add(this, tag);
            transaction.commitAllowingStateLoss();
        } catch (IllegalStateException e) {
            Log.e(TAG, e.getMessage());
        }
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        ((BottomSheetDialog) getDialog())
                .getBehavior()
                .setState(BottomSheetBehavior.STATE_EXPANDED);

        clickOnHappyImageView(view);
        clickOnSadImageView(view);
    }

    @Nullable
    @Override
    public View onCreateView(
            @NonNull LayoutInflater inflater,
            @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        return inflater.inflate(R.layout.brave_rating_dialog_fragment, container, false);
    }

    private void clickOnHappyImageView(View view) {
        ImageView happyImageView = view.findViewById(R.id.happyImageView);
        happyImageView.setOnClickListener(
                (v) -> {
                    dismiss();
                    BraveAskPlayStoreRatingDialog fragment =
                            BraveAskPlayStoreRatingDialog.newInstance(mIsFromSettings);
                    fragment.show(
                            ((AppCompatActivity) getActivity()).getSupportFragmentManager(),
                            BraveAskPlayStoreRatingDialog.TAG_FRAGMENT);
                });
    }

    private void clickOnSadImageView(View view) {
        ImageView sadImageView = view.findViewById(R.id.sadImageView);
        sadImageView.setOnClickListener((v) -> {
            dismiss();
            BraveRateThanksFeedbackDialog.showBraveRateThanksFeedbackDialog(
                    (AppCompatActivity) getActivity());
        });
    }
}
