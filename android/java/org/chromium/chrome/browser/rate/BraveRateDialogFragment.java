/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.rate;

import android.content.Context;
import android.content.DialogInterface;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CheckBox;
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

    // Mirrors the "Don't show again" checkbox so the preference can be written from onDismiss
    // without holding on to a view past its lifecycle.
    private boolean mDontShowAgain;

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
        setStyle(STYLE_NORMAL, R.style.RatingBottomSheetDialogTheme);
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
        BottomSheetBehavior<?> behavior = ((BottomSheetDialog) getDialog()).getBehavior();
        behavior.setState(BottomSheetBehavior.STATE_EXPANDED);
        behavior.setMaxWidth(getResources().getDimensionPixelSize(R.dimen.bottom_sheet_max_width));

        clickOnHappyImageView(view);
        clickOnSadImageView(view);
        setUpDontShowAgainRow(view);
    }

    @Override
    public void onDismiss(@NonNull DialogInterface dialog) {
        // Persisted once the whole prompt goes away, so it covers every exit path: an emoji
        // choice, a swipe down or a tap outside. The current state is written rather than only
        // the opt-out, so unticking the box clears the preference again.
        RateUtils.getInstance().setPrefRateDontShowAgain(mDontShowAgain);
        super.onDismiss(dialog);
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

    private void setUpDontShowAgainRow(View view) {
        CheckBox dontShowAgainCheckBox = view.findViewById(R.id.dont_show_again_checkbox);
        mDontShowAgain = RateUtils.getInstance().getPrefRateDontShowAgain();
        dontShowAgainCheckBox.setChecked(mDontShowAgain);
        dontShowAgainCheckBox.setOnCheckedChangeListener(
                (buttonView, isChecked) -> mDontShowAgain = isChecked);
        // The checkbox is not clickable itself, so the whole row drives it.
        view.findViewById(R.id.dont_show_again_row)
                .setOnClickListener(
                        (v) ->
                                dontShowAgainCheckBox.setChecked(
                                        !dontShowAgainCheckBox.isChecked()));
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
