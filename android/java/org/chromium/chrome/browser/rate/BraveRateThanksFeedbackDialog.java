/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.rate;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.Dialog;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.Button;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentTransaction;

import com.google.android.material.bottomsheet.BottomSheetBehavior;
import com.google.android.material.bottomsheet.BottomSheetDialog;
import com.google.android.material.bottomsheet.BottomSheetDialogFragment;
import com.google.android.material.textfield.TextInputEditText;

import org.chromium.base.Log;
import org.chromium.base.task.AsyncTask;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ui.messages.snackbar.Snackbar;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager;

public class BraveRateThanksFeedbackDialog extends BottomSheetDialogFragment {
    public static final String TAG_FRAGMENT = "brave_rate_thanks_feedback_dialog_tag";
    private static final String TAG = "RateThanksFeedback";
    private static final String SAD = "sad";

    public static BraveRateThanksFeedbackDialog newInstance() {
        return new BraveRateThanksFeedbackDialog();
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setStyle(STYLE_NORMAL, R.style.RatingBottomSheetDialogTheme);
    }

    @Override
    public void show(@NonNull FragmentManager manager, @Nullable String tag) {
        try {
            BraveRateThanksFeedbackDialog fragment =
                    (BraveRateThanksFeedbackDialog) manager.findFragmentByTag(
                            BraveRateThanksFeedbackDialog.TAG_FRAGMENT);
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

        // Inflate against the dialog rather than the fragment so the sheet theme applies to the
        // content; the layout resolves its colors from theme attributes.
        final View view =
                LayoutInflater.from(dialog.getContext())
                        .inflate(R.layout.brave_rating_thanks_feedback_dialog, null);
        addSuggestionEditText(view);
        clickOnSendButton(view);
        clickOnCancelButton(view);
        dialog.setContentView(view);

        // Expand straight away: the default peek height is derived from the parent's width, so in
        // landscape it leaves only a sliver of the sheet on screen. skipCollapsed also stops the
        // sheet dropping back to that peek when the window resizes for the keyboard.
        BottomSheetBehavior<?> behavior = ((BottomSheetDialog) dialog).getBehavior();
        behavior.setState(BottomSheetBehavior.STATE_EXPANDED);
        behavior.setSkipCollapsed(true);
        behavior.setMaxWidth(getResources().getDimensionPixelSize(R.dimen.bottom_sheet_max_width));
    }

    private void clickOnCancelButton(View view) {
        Button cancelButton = view.findViewById(R.id.rate_cancel_button);
        cancelButton.setOnClickListener(
                (v) -> {
                    RateUtils.getInstance().setPrefNextRateDate();
                    dismiss();
                });
    }

    private void clickOnSendButton(View view) {
        Button sendButton = view.findViewById(R.id.rate_send_button);
        sendButton.setOnClickListener(
                (v) -> {
                    TextInputEditText feedbackEditText = view.findViewById(R.id.feedbackEditText);
                    String feedBack = feedbackEditText.getText().toString();
                    RateFeedbackUtils.RateFeedbackWorkerTask workerTask =
                            new RateFeedbackUtils.RateFeedbackWorkerTask(
                                    SAD, feedBack, mRateFeedbackCallback);
                    workerTask.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
                });
    }

    private void addSuggestionEditText(View view) {
        TextInputEditText suggestion = view.findViewById(R.id.feedbackEditText);
        suggestion.requestFocus();
    }

    public static void showBraveRateThanksFeedbackDialog(AppCompatActivity activity) {
        if (activity != null) {
            BraveRateThanksFeedbackDialog braveAskPlayStoreRatingDialog =
                    BraveRateThanksFeedbackDialog.newInstance();
            braveAskPlayStoreRatingDialog.show(activity.getSupportFragmentManager(), TAG_FRAGMENT);
        }
    }

    /**
     * Shows the "Feedback sent" confirmation over the browser. Must run before {@link #dismiss()},
     * which detaches the fragment and leaves {@link #getActivity()} null.
     */
    private void showFeedbackSentSnackbar() {
        Activity activity = getActivity();
        if (!(activity instanceof SnackbarManager.SnackbarManageable)) {
            return;
        }
        SnackbarManager snackbarManager =
                ((SnackbarManager.SnackbarManageable) activity).getSnackbarManager();
        if (snackbarManager == null) {
            return;
        }

        // Chromium's default snackbar styling is used deliberately - the confirmation is
        // informational and goes away on the TYPE_NOTIFICATION timeout or a swipe.
        Snackbar snackbar =
                Snackbar.make(
                        activity.getString(R.string.brave_rating_feedback_sent),
                        /* controller= */ null,
                        Snackbar.TYPE_NOTIFICATION,
                        Snackbar.UMA_UNKNOWN);
        snackbarManager.showSnackbar(snackbar);
    }

    private final RateFeedbackUtils.RateFeedbackCallback mRateFeedbackCallback =
            new RateFeedbackUtils.RateFeedbackCallback() {
                @Override
                public void rateFeedbackSubmitted() {
                    RateUtils.getInstance().setPrefNextRateDate();
                    showFeedbackSentSnackbar();
                    dismiss();
                }
            };
}
