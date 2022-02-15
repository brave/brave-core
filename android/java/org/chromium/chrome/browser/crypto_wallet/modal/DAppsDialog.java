/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.modal;

import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
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

public class DAppsDialog extends Dialog implements ConnectionErrorHandler, KeyringServiceObserver {
    public static final String TAG_FRAGMENT = DAppsDialog.class.getName();

    private Button mbtUnlock;
    private boolean mShowOnboarding;
    private KeyringService mKeyringService;
    private DialogInterface.OnDismissListener mOnDismissListener;
    private boolean mDismissed;

    public DAppsDialog(
            @NonNull Context context, DialogInterface.OnDismissListener onDismissListener) {
        super(context, R.style.BraveWalletDialog);
        mDismissed = false;
        mOnDismissListener = onDismissListener;
    }

    public static DAppsDialog newInstance(
            Context context, DialogInterface.OnDismissListener onDismissListener) {
        return new DAppsDialog(context, onDismissListener);
    }

    public void showOnboarding(boolean showOnboarding) {
        mShowOnboarding = showOnboarding;
        show();
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        setContentView(R.layout.dapps_bottom_sheet);
        initKeyringService();

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
                dismiss();
            } else {
                mKeyringService.isLocked(isLocked -> {
                    if (isLocked) {
                        openWallet();
                    } else {
                        // Todo: show option to connect account
                    }
                    dismiss();
                });
            }
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
            mKeyringService = null;
        }
        mDismissed = true;
        if (mOnDismissListener != null) {
            mOnDismissListener.onDismiss(this);
        }
    }

    private void updateView() {
        assert mbtUnlock != null;
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
        if (mKeyringService != null) {
            mKeyringService.close();
            mKeyringService = null;
        }
        // The connection error often occures when the dialog is already
        // dismissed.
        if (!mDismissed) {
            initKeyringService();
        }
    }

    private void initKeyringService() {
        if (mKeyringService != null) {
            return;
        }

        mKeyringService = KeyringServiceFactory.getInstance().getKeyringService(this);
    }
}
