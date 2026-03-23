/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.custom_search_engines;

import android.app.Dialog;
import android.content.Context;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.Button;
import android.widget.TextView;

import org.chromium.build.annotations.Initializer;
import org.chromium.build.annotations.NullMarked;

/**
 * A dialog class that handles confirmation dialogs in Brave browser. Provides functionality for
 * creating and showing custom dialogs with configurable views and buttons. This class encapsulates
 * all the dialog creation, configuration and display logic.
 */
@NullMarked
public class ConfirmationDialog {

    /** Interface for handling confirmation dialog button clicks. */
    public interface OnConfirmationDialogListener {
        /** Called when the user clicks the confirm/positive button. */
        void onPositiveButtonClicked();

        /** Called when the user clicks the cancel/negative button. */
        void onNegativeButtonClicked();
    }

    private Dialog mDialog;
    private Context mContext;
    private OnConfirmationDialogListener mListener;

    /**
     * Shows a confirmation dialog with customizable title, message and button text.
     *
     * @param context The Android context
     * @param title The title text to show in the dialog header
     * @param message The message text to show in the dialog body
     * @param positiveButtonText The text for the positive/confirm button
     * @param negativeButtonText The text for the negative/cancel button
     * @param listener Callback interface for button click events
     */
    @Initializer
    public void showConfirmDialog(
            Context context,
            String title,
            CharSequence message,
            String positiveButtonText,
            String negativeButtonText,
            OnConfirmationDialogListener listener) {
        mContext = context;
        mListener = listener;

        initializeDialog();
        configureDialogContent(title, message);
        configureDialogButtons(positiveButtonText, negativeButtonText);
        showConfiguredDialog();
    }

    private void initializeDialog() {
        if (mContext == null) {
            return;
        }
        mDialog = new Dialog(mContext);
        mDialog.requestWindowFeature(Window.FEATURE_NO_TITLE);
        Window window = mDialog.getWindow();
        if (window != null) {
            window.setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));
        }
        mDialog.setContentView(R.layout.confirmation_dialog_layout);
    }

    private void configureDialogContent(String title, CharSequence message) {
        if (mDialog == null) {
            return;
        }
        TextView titleView = mDialog.findViewById(R.id.dialogTitle);
        TextView messageView = mDialog.findViewById(R.id.dialogMessage);
        if (titleView != null) {
            titleView.setText(title);
        }
        if (messageView != null) {
            messageView.setText(message);
        }
    }

    private void configureDialogButtons(String positiveText, String negativeText) {
        if (mDialog == null) {
            return;
        }
        Button positiveButton = mDialog.findViewById(R.id.positiveButton);
        Button negativeButton = mDialog.findViewById(R.id.negativeButton);

        if (positiveButton != null) {
            positiveButton.setText(positiveText);
            positiveButton.setOnClickListener(v -> handlePositiveClick());
        }
        if (negativeButton != null) {
            negativeButton.setText(negativeText);
            negativeButton.setOnClickListener(v -> handleNegativeClick());
        }
    }

    private void handlePositiveClick() {
        if (mListener != null) {
            mListener.onPositiveButtonClicked();
        }
        if (mDialog != null) {
            mDialog.dismiss();
        }
    }

    private void handleNegativeClick() {
        if (mListener != null) {
            mListener.onNegativeButtonClicked();
        }
        if (mDialog != null) {
            mDialog.dismiss();
        }
    }

    private void showConfiguredDialog() {
        if (mDialog == null) {
            return;
        }
        mDialog.show();
        Window window = mDialog.getWindow();
        if (window != null) {
            window.setLayout(
                    ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        }
    }
}
