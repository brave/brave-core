/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.modal;

import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.Gravity;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;

import androidx.annotation.IntDef;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.crypto_wallet.BraveWalletServiceFactory;
import org.chromium.chrome.browser.crypto_wallet.util.AndroidUtils;
import org.chromium.chrome.browser.crypto_wallet.util.WalletConstants;
import org.chromium.chrome.browser.util.ConfigurationUtils;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;

public class DAppsDialog extends Dialog implements ConnectionErrorHandler {
    private static final String TAG = "DAppsDialog";

    public static final String TAG_FRAGMENT = DAppsDialog.class.getName();

    private View mRootView;
    private boolean mShowOnboarding;
    private KeyringService mKeyringService;
    private DialogInterface.OnDismissListener mOnDismissListener;
    private boolean mDismissed;
    private final Handler mHandler;
    @DAppsDialogStyle private final int mStyle;

    public DAppsDialog(
            @NonNull Context context,
            DialogInterface.OnDismissListener onDismissListener,
            @DAppsDialogStyle int dialogStyle) {
        super(context, getDialogTheme(dialogStyle));
        mDismissed = false;
        mOnDismissListener = onDismissListener;
        mStyle = dialogStyle;
        mHandler = new Handler(Looper.getMainLooper());
    }

    public static DAppsDialog newInstance(
            Context context,
            DialogInterface.OnDismissListener onDismissListener,
            @DAppsDialogStyle int dialogStyle) {
        return new DAppsDialog(context, onDismissListener, dialogStyle);
    }

    public void showOnboarding(boolean showOnboarding) {
        mShowOnboarding = showOnboarding;
        show();
        mHandler.postDelayed(
                () -> {
                    if (isShowing()) {
                        dismiss();
                    }
                },
                WalletConstants.MILLI_SECOND * 30);
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        setContentView(R.layout.dapps_dialog);
        initKeyringService();
        Window window = getWindow();
        WindowManager.LayoutParams wlp = window.getAttributes();
        wlp.gravity = getDialogThemeGravity(mStyle);
        // Add additional height (half of toolbar) on tablet to cover the toolbar by the DApp dialog
        if (ConfigurationUtils.isTablet(getContext())) {
            wlp.y = wlp.y + (AndroidUtils.getToolBarHeight(getContext()) / 2);
        }
        window.setAttributes(wlp);
        window.clearFlags(WindowManager.LayoutParams.FLAG_DIM_BEHIND);
        window.setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));
        mRootView = findViewById(R.id.dapp_dialog_root);
        mRootView.setOnClickListener(
                v -> {
                    if (mShowOnboarding) {
                        openWallet();
                        dismiss();
                    } else {
                        mKeyringService.isLocked(
                                isLocked -> {
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
        try {
            BraveActivity activity = BraveActivity.getBraveActivity();
            activity.openBraveWallet(true, false, false);
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "openWallet " + e);
        }
    }

    @Override
    public void dismiss() {
        super.dismiss();
        mHandler.removeCallbacksAndMessages(null);
        if (mKeyringService != null) {
            mKeyringService.close();
            mKeyringService = null;
        }
        mDismissed = true;
        if (mOnDismissListener != null) {
            mOnDismissListener.onDismiss(this);
        }
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
        mKeyringService = BraveWalletServiceFactory.getInstance().getKeyringService(this);
    }

    private static int getDialogThemeGravity(@DAppsDialogStyle int mStyle) {
        switch (mStyle) {
            case DAppsDialogStyle.TOP:
                return Gravity.TOP;
            case DAppsDialogStyle.BOTTOM:
            default:
                return Gravity.BOTTOM;
        }
    }

    private static int getDialogTheme(@DAppsDialogStyle int mStyle) {
        switch (mStyle) {
            case DAppsDialogStyle.TOP:
                return R.style.BraveWalletDAppNotificationDialogTop;
            case DAppsDialogStyle.BOTTOM:
            default:
                return R.style.BraveWalletDAppNotificationDialogBottom;
        }
    }

    @IntDef({DAppsDialogStyle.TOP, DAppsDialogStyle.BOTTOM})
    public @interface DAppsDialogStyle {
        int TOP = 0;
        int BOTTOM = 1;
    }
}
