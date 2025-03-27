/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.utils.confirm_dialog;

import android.app.Dialog;
import android.content.Context;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.Button;
import android.widget.TextView;

import org.chromium.brave.browser.utils.R;

/**
 * A dialog class that handles confirmation dialogs in Brave browser. Provides functionality for
 * creating and showing custom dialogs with configurable views and buttons. This class encapsulates
 * all the dialog creation, configuration and display logic.
 */
public class BraveConfirmationDialog {
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
    public void showConfirmDialog(
            Context context,
            String title,
            String message,
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
        mDialog = new Dialog(mContext);
        mDialog.requestWindowFeature(Window.FEATURE_NO_TITLE);
        mDialog.getWindow().setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));
        mDialog.setContentView(R.layout.brave_confirmation_dialog_layout);
    }

    private void configureDialogContent(String title, String message) {
        TextView titleView = mDialog.findViewById(R.id.dialogTitle);
        TextView messageView = mDialog.findViewById(R.id.dialogMessage);
        titleView.setText(title);
        messageView.setText(message);
    }

    private void configureDialogButtons(String positiveText, String negativeText) {
        Button positiveButton = mDialog.findViewById(R.id.positiveButton);
        Button negativeButton = mDialog.findViewById(R.id.negativeButton);

        positiveButton.setText(positiveText);
        negativeButton.setText(negativeText);

        positiveButton.setOnClickListener(v -> handlePositiveClick());
        negativeButton.setOnClickListener(v -> handleNegativeClick());
    }

    private void handlePositiveClick() {
        mListener.onPositiveButtonClicked();
        mDialog.dismiss();
    }

    private void handleNegativeClick() {
        mListener.onNegativeButtonClicked();
        mDialog.dismiss();
    }

    private void showConfiguredDialog() {
        mDialog.show();
        Window window = mDialog.getWindow();
        window.setLayout(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
    }
}
