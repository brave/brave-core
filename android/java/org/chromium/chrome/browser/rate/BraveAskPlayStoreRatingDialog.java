/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.rate;

import android.annotation.SuppressLint;
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

    public static BraveAskPlayStoreRatingDialog newInstance() {
        return new BraveAskPlayStoreRatingDialog();
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

        final View view = LayoutInflater.from(getContext())
                                  .inflate(R.layout.brave_ask_play_store_rating_dialog, null);
        clickRateNowButton(view);
        clickNotNowButton(view);
        dialog.setContentView(view);
    }

    private void clickRateNowButton(View view) {
        Button rateNowButton = view.findViewById(R.id.rate_now_button);
        rateNowButton.setOnClickListener((v) -> {
            rating(getActivity());
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

    public static void showBraveAskPlayStoreRatingDialog(AppCompatActivity activity) {
        if (activity != null) {
            BraveAskPlayStoreRatingDialog braveAskPlayStoreRatingDialog =
                    BraveAskPlayStoreRatingDialog.newInstance();
            braveAskPlayStoreRatingDialog.show(activity.getSupportFragmentManager(), TAG_FRAGMENT);
        }
    }

    private void rating(Context context) {
        ReviewManager manager = ReviewManagerFactory.create(context);
        Task<ReviewInfo> request = manager.requestReviewFlow();
        request.addOnCompleteListener(task -> {
            if (task.isSuccessful()) {
                // We can get the ReviewInfo object
                ReviewInfo reviewInfo = task.getResult();
                Task<Void> flow = manager.launchReviewFlow(getActivity(), reviewInfo);
                flow.addOnCompleteListener(task1
                        -> {
                                // The flow has finished. The API does not indicate whether the user
                                // reviewed or not, or even whether the review dialog was shown.
                                // Thus, no
                                // matter the result, we continue our app flow.
                        });
            } else {
                // There was some problem
                Log.e(TAG, "In-App review error " + task.getException());
            }
        });
    }
}
