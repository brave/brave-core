/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.notifications.permissions;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.res.Resources;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import androidx.annotation.NonNull;

import org.chromium.base.Callback;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.crypto_wallet.util.WalletConstants;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.notifications.NotificationUmaTracker;
import org.chromium.chrome.browser.notifications.NotificationUmaTracker.NotificationRationaleResult;
import org.chromium.chrome.browser.notifications.permissions.NotificationPermissionController.RationaleDelegate;
import org.chromium.components.browser_ui.modaldialog.ModalDialogView;
import org.chromium.ui.modaldialog.DialogDismissalCause;
import org.chromium.ui.modaldialog.ModalDialogManager;
import org.chromium.ui.modaldialog.ModalDialogManager.ModalDialogType;
import org.chromium.ui.modaldialog.ModalDialogProperties;
import org.chromium.ui.modaldialog.ModalDialogProperties.ButtonStyles;
import org.chromium.ui.modaldialog.SimpleModalDialogController;
import org.chromium.ui.modelutil.PropertyModel;

public class BraveNotificationPermissionRationaleDialogController
        extends NotificationPermissionRationaleDialogController {
    private final ModalDialogManager mModalDialogManager;
    private final Context mContext;

    public BraveNotificationPermissionRationaleDialogController(
            Context context, ModalDialogManager modalDialogManager) {
        super(context, modalDialogManager);
        mModalDialogManager = modalDialogManager;
        mContext = context;
    }

    @Override
    public void showRationaleUi(Callback<Boolean> rationaleCallback) {
        LayoutInflater inflater = LayoutInflater.from(mContext);
        Resources resources = mContext.getResources();

        View dialogView = inflater.inflate(R.layout.brave_notification_permission_rationale_dialog,
                /* root= */ null);
        TextView titleView = dialogView.findViewById(R.id.notification_permission_rationale_title);
        TextView descriptionView =
                dialogView.findViewById(R.id.notification_permission_rationale_message);

        PropertyModel.Builder dialogModelBuilder =
                new PropertyModel.Builder(ModalDialogProperties.ALL_KEYS)
                        .with(ModalDialogProperties.CONTROLLER,
                                new SimpleModalDialogController(mModalDialogManager,
                                        wrapDialogDismissalCallback(rationaleCallback)))
                        .with(ModalDialogProperties.CUSTOM_VIEW, dialogView);

        PropertyModel dialogModel = dialogModelBuilder.build();

        // continue button
        Button btnContinue = dialogView.findViewById(R.id.notification_continue_button);
        btnContinue.setOnClickListener((v) -> {
            mModalDialogManager.dismissDialog(
                    dialogModel, DialogDismissalCause.POSITIVE_BUTTON_CLICKED);
        });

        // now Button
        Button btnNotNow = dialogView.findViewById(R.id.notification_not_now_button);
        btnNotNow.setOnClickListener((v) -> {
            mModalDialogManager.dismissDialog(
                    dialogModel, DialogDismissalCause.NEGATIVE_BUTTON_CLICKED);
        });

        mModalDialogManager.showDialog(dialogModel, ModalDialogType.APP);
    }

    private Callback<Integer> wrapDialogDismissalCallback(Callback<Boolean> rationaleCallback) {
        assert false : "removeSuggestionsForGroup should be redirected to parent in bytecode!";
        return null;
    }
}
