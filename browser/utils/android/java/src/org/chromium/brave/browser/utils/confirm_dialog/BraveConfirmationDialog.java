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
    /**
     * Shows a confirmation dialog with customizable title, message and button text. Creates and
     * configures a dialog with the provided parameters and displays it.
     *
     * @param title The title text to show in the dialog header
     * @param message The message text to show in the dialog body
     * @param positiveButtonText The text to display on the positive/confirm button
     * @param negativeButtonText The text to display on the negative/cancel button
     * @param listener Callback interface to handle button click events
     */
    public void showConfirmDialog(
            Context context,
            String title,
            String message,
            String positiveButtonText,
            String negativeButtonText,
            OnConfirmationDialogListener listener) {
        final Dialog dialog = createDialog(context);
        setupDialogViews(dialog, title, message);
        setupDialogButtons(dialog, positiveButtonText, negativeButtonText, listener);
        showDialog(dialog);
    }

    /**
     * Creates and initializes a new dialog instance. Sets up basic dialog properties like window
     * features and layout. Configures the dialog with a transparent background and custom layout.
     *
     * @return A configured Dialog instance ready for content setup
     */
    private Dialog createDialog(Context context) {
        final Dialog dialog = new Dialog(context);
        dialog.requestWindowFeature(Window.FEATURE_NO_TITLE);
        dialog.getWindow().setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));
        dialog.setContentView(R.layout.brave_confirmation_dialog_layout);
        return dialog;
    }

    /**
     * Sets up the dialog views including title and message. Finds and configures the title and
     * message TextViews in the dialog layout.
     *
     * @param dialog The dialog whose views need to be configured
     * @param title The title text to set
     * @param message The message text to set
     */
    private void setupDialogViews(Dialog dialog, String title, String message) {
        TextView titleTextView = dialog.findViewById(R.id.dialogTitle);
        TextView messageTextView = dialog.findViewById(R.id.dialogMessage);

        titleTextView.setText(title);
        messageTextView.setText(message);
    }

    /**
     * Sets up the dialog buttons and their click listeners. Configures the positive and negative
     * buttons with text and click handlers. When buttons are clicked, the appropriate listener
     * method is called and dialog is dismissed.
     *
     * @param dialog The dialog whose buttons need to be configured
     * @param positiveButtonText The text for the positive/confirm button
     * @param negativeButtonText The text for the negative/cancel button
     * @param listener The callback interface for button click events
     */
    private void setupDialogButtons(
            Dialog dialog,
            String positiveButtonText,
            String negativeButtonText,
            OnConfirmationDialogListener listener) {
        Button positiveButton = dialog.findViewById(R.id.positiveButton);
        Button negativeButton = dialog.findViewById(R.id.negativeButton);

        positiveButton.setText(positiveButtonText);
        negativeButton.setText(negativeButtonText);

        positiveButton.setOnClickListener(
                v -> {
                    listener.onPositiveButtonClicked();
                    dialog.dismiss();
                });

        negativeButton.setOnClickListener(
                v -> {
                    listener.onNegativeButtonClicked();
                    dialog.dismiss();
                });
    }

    /**
     * Shows the configured dialog and sets its layout parameters. Displays the dialog and
     * configures its window to match parent width while wrapping content height.
     *
     * @param dialog The fully configured dialog to be displayed
     */
    private void showDialog(Dialog dialog) {
        dialog.show();
        Window window = dialog.getWindow();
        window.setLayout(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
    }
}
