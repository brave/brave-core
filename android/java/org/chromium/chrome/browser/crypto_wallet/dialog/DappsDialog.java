/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.dialog;

import android.app.Dialog;
import android.content.Context;
import android.os.Bundle;
import android.view.Gravity;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.crypto_wallet.KeyringServiceFactory;
import org.chromium.chrome.browser.crypto_wallet.observers.KeyringServiceObserver;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;
import org.chromium.ui.modaldialog.ModalDialogManager;
import org.chromium.ui.modelutil.PropertyModel;

public class DappsDialog extends Dialog implements ConnectionErrorHandler, KeyringServiceObserver {
    public static final String TAG_FRAGMENT = DappsDialog.class.getName();

    private Button mbtUnlock;
    private boolean mShowOnboarding;
    private KeyringService mKeyringService;
    private ModalDialogManager mManager;
    private PropertyModel mModel;
    private Dialog mDialog;

    public DappsDialog(@NonNull Context context) {
        super(context, R.style.BraveWalletDialog);
    }

    public static DappsDialog newInstance(Context context) {
        return new DappsDialog(context);
    }

    public void showOnboarding(boolean showOnboarding) {
        mShowOnboarding = showOnboarding;
        show();
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        initKeyringService();
        setContentView(R.layout.dapps_bottom_sheet);

        Window window = getWindow();
        WindowManager.LayoutParams wlp = window.getAttributes();
        wlp.gravity = Gravity.BOTTOM;
        wlp.flags &= ~WindowManager.LayoutParams.FLAG_DIM_BEHIND;
        window.setAttributes(wlp);

        TextView tvDappUrl = findViewById(R.id.tv_dapp_url);
        mbtUnlock = findViewById(R.id.unlock);

        tvDappUrl.setText(getCurrentHostHttpAddress());
        updateView();
        mbtUnlock.setOnClickListener(v -> {
            if (mShowOnboarding) {
                openWallet();
            } else {
                mKeyringService.isLocked(isLocked -> {
                    if (isLocked) {
                        openWallet();
                    } else {
                        // Todo: show option to connect account
                    }
                });
            }
            dismiss();
        });
    }

    private void openWallet() {
        BraveActivity activity = BraveActivity.getBraveActivity();
        assert activity != null;
        activity.openBraveWallet(true);
    }

    @Override
    public void dismiss() {
        super.dismiss();
        if (mKeyringService != null) {
            mKeyringService.close();
        }
    }

    private void updateView() {
        if (mShowOnboarding) {
            mbtUnlock.setText(getContext().getString(R.string.setup_crypto));
        } else if (mKeyringService != null) {
            mKeyringService.isLocked(isLocked -> {
                if (isLocked) {
                    mbtUnlock.setText(getContext().getString(R.string.unlock));
                } else {
                    mbtUnlock.setText(getContext().getString(R.string.continue_button));
                }
            });
        }
    }

    private String getCurrentHostHttpAddress() {
        ChromeTabbedActivity activity = BraveActivity.getChromeTabbedActivity();
        if (activity != null) {
            return activity.getActivityTab().getUrl().getSpec();
        }
        return "";
    }

    @Override
    public void onConnectionError(MojoException e) {
        mKeyringService.close();
        mKeyringService = null;
        initKeyringService();
    }

    private void initKeyringService() {
        if (mKeyringService != null) {
            return;
        }

        mKeyringService = KeyringServiceFactory.getInstance().getKeyringService(this);
        updateView();
    }
}
