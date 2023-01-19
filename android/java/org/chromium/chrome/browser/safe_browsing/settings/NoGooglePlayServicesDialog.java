/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.safe_browsing.settings;

import android.content.Context;
import android.content.res.Resources;

import org.chromium.components.browser_ui.modaldialog.AppModalPresenter;
import org.chromium.ui.modaldialog.DialogDismissalCause;
import org.chromium.ui.modaldialog.ModalDialogManager;
import org.chromium.ui.modaldialog.ModalDialogManager.ModalDialogType;
import org.chromium.ui.modaldialog.ModalDialogProperties;
import org.chromium.ui.modaldialog.ModalDialogProperties.Controller;
import org.chromium.ui.modelutil.PropertyModel;

/**
 * Dialog to notify that the user is not protected by Safe Browsing as Google Play Services are not
 * available.
 */
public class NoGooglePlayServicesDialog {
    private Context mContext;
    private ModalDialogManager mManager;
    private PropertyModel mModel;

    public static NoGooglePlayServicesDialog create(Context context) {
        return new NoGooglePlayServicesDialog(context);
    }

    private NoGooglePlayServicesDialog(Context context) {
        mContext = context;
    }

    /** Show this dialog in the context of its enclosing activity. */
    public void show() {
        Resources resources = mContext.getResources();
        PropertyModel.Builder builder =
                new PropertyModel.Builder(ModalDialogProperties.ALL_KEYS)
                        .with(ModalDialogProperties.CONTROLLER, makeController())
                        .with(ModalDialogProperties.TITLE, resources,
                                R.string.safe_browsing_no_google_play_services_dialog_title)
                        .with(ModalDialogProperties.MESSAGE_PARAGRAPH_1,
                                resources.getString(
                                        R.string.safe_browsing_no_google_play_services_dialog_message))
                        .with(ModalDialogProperties.POSITIVE_BUTTON_TEXT, resources, R.string.ok);
        mModel = builder.build();
        mManager = new ModalDialogManager(new AppModalPresenter(mContext), ModalDialogType.APP);
        mManager.showDialog(mModel, ModalDialogType.APP);
    }

    private Controller makeController() {
        return new ModalDialogProperties.Controller() {
            @Override
            public void onClick(PropertyModel model, int buttonType) {
                mManager.dismissDialog(mModel, DialogDismissalCause.POSITIVE_BUTTON_CLICKED);
            }

            @Override
            public void onDismiss(PropertyModel model, int dismissalCause) {
                mManager.destroy();
            }
        };
    }
}
