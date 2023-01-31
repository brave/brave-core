/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.rate;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.Dialog;
import android.content.Context;
import android.os.Bundle;
import android.os.Handler;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.Button;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.AppCompatButton;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentTransaction;

import com.google.android.material.bottomsheet.BottomSheetDialogFragment;
import com.google.android.play.core.review.ReviewInfo;
import com.google.android.play.core.review.ReviewManager;
import com.google.android.play.core.review.ReviewManagerFactory;
import com.google.android.play.core.tasks.Task;

import org.chromium.base.Log;
import org.chromium.chrome.R;

public class BraveAskPlayStoreRatingDialog extends BottomSheetDialogFragment {
    final public static String TAG_FRAGMENT = "brave_ask_play_store_rating_dialog_tag";
    private static final String TAG = "AskPlayStoreRating";
    private ReviewManager mReviewManager;
    private ReviewInfo mReviewInfo;
    private boolean mIsFromSettings;
    private Context mContext;

    public static BraveAskPlayStoreRatingDialog newInstance(boolean isFromSettings) {
        Bundle bundle = new Bundle();
        bundle.putBoolean(RateUtils.FROM_SETTINGS, isFromSettings);
        BraveAskPlayStoreRatingDialog fragment = new BraveAskPlayStoreRatingDialog();
        fragment.setArguments(bundle);

        return fragment;
    }

    @Override
    public void onAttach(@NonNull Context context) {
        super.onAttach(context);
        mContext = context;
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
            BraveAskPlayStoreRatingDialog fragment =
                    (BraveAskPlayStoreRatingDialog) manager.findFragmentByTag(
                            BraveAskPlayStoreRatingDialog.TAG_FRAGMENT);
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

    @SuppressLint("RestrictedApi")
    @Override
    public void setupDialog(@NonNull Dialog dialog, int style) {
        super.setupDialog(dialog, style);
        mReviewManager = ReviewManagerFactory.create(mContext);
        try {
            requestReviewFlow();
        } catch (NullPointerException e) {
            Log.e(TAG, "In-App requestReviewFlow exception");
        }

        final View view = LayoutInflater.from(getContext())
                                  .inflate(R.layout.brave_ask_play_store_rating_dialog, null);
        clickRateNowButton(view);
        clickNotNowButton(view);
        dialog.setContentView(view);
    }

    private void clickRateNowButton(View view) {
        Button rateNowButton = view.findViewById(R.id.rate_now_button);
        rateNowButton.setOnClickListener((v) -> {
            try {
                if (mIsFromSettings) {
                    RateUtils.getInstance().openPlaystore(mContext);
                } else {
                    launchReviewFlow();
                }
            } catch (NullPointerException e) {
                Log.e(TAG, "In-App launch Review exception");
            }
            new Handler().postDelayed(new Runnable() {
                @Override
                public void run() {
                    dismiss();
                }
            }, 500);
        });
    }

    private void clickNotNowButton(View view) {
        Button rateNotNowButton = view.findViewById(R.id.rate_not_now_button);
        rateNotNowButton.setOnClickListener((v) -> dismiss());
    }

    private void requestReviewFlow() {
        if (mReviewManager != null) {
            Task<ReviewInfo> request = mReviewManager.requestReviewFlow();
            request.addOnCompleteListener(task -> {
                if (task.isSuccessful()) {
                    mReviewInfo = task.getResult();
                } else {
                    // There was some problem
                    Log.e(TAG, "In-App review error " + task.getException());
                }
            });
        }
    }

    private void launchReviewFlow() {
        if (mReviewManager != null && mReviewInfo != null && mContext instanceof Activity) {
            // We can get the ReviewInfo object
            Task<Void> flow = mReviewManager.launchReviewFlow((Activity) mContext, mReviewInfo);
            flow.addOnCompleteListener(task1
                    -> {
                            // The flow has finished. The API does not indicate whether the user
                            // reviewed or not, or even whether the review dialog was shown.
                            // Thus, no matter the result, we continue our app flow.
                    });
        } else {
            // if case fails then open play store app page
            RateUtils.getInstance().openPlaystore(mContext);
        }
    }
}
