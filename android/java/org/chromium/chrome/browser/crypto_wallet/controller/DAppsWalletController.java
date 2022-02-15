/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.controller;

import android.content.Context;
import android.content.DialogInterface;
import android.view.View;

import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.chrome.browser.crypto_wallet.KeyringServiceFactory;
import org.chromium.chrome.browser.crypto_wallet.modal.BraveWalletPanel;
import org.chromium.chrome.browser.crypto_wallet.modal.DAppsDialog;
import org.chromium.chrome.browser.crypto_wallet.observers.KeyringServiceObserver;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;

public class DAppsWalletController implements ConnectionErrorHandler, KeyringServiceObserver {
    private Context mContext;
    private View mAnchorViewHost;
    private KeyringService mKeyringService;
    private boolean mHasStateInitialise;
    private DAppsDialog mDAppsDialog;
    private BraveWalletPanel mBraveWalletPanel;
    private DialogInterface.OnDismissListener mOnDismissListener;
    private DialogInterface.OnDismissListener mDialogOrPanelDismissListener = dialog -> {
        if (mOnDismissListener != null) {
            mOnDismissListener.onDismiss(dialog);
        }
        cleanUp();
    };

    public DAppsWalletController(Context mContext, View mAnchorViewHost) {
        this.mContext = mContext;
        this.mAnchorViewHost = mAnchorViewHost;
    }

    public DAppsWalletController(Context mContext, View mAnchorViewHost,
            DialogInterface.OnDismissListener onDismissListener) {
        this(mContext, mAnchorViewHost);
        this.mOnDismissListener = onDismissListener;
    }

    public void showWalletPanel() {
        initKeyringService();
        if (Utils.shouldShowCryptoOnboarding()) {
            showOnBoardingOrUnlock();
        } else {
            mKeyringService.isLocked(isLocked -> {
                if (isLocked) {
                    showOnBoardingOrUnlock();
                } else {
                    boolean isFoundPendingDAppsTx = false;
                    // TODO: check if pending dapps transaction are available and implement an
                    // action accrodingly
                    if (!isFoundPendingDAppsTx) {
                        mBraveWalletPanel = new BraveWalletPanel(
                                mAnchorViewHost, mDialogOrPanelDismissListener);
                        mBraveWalletPanel.showLikePopDownMenu();
                    }
                }
            });
        }
    }

    private void showOnBoardingOrUnlock() {
        mDAppsDialog = DAppsDialog.newInstance(mContext, mDialogOrPanelDismissListener);
        mDAppsDialog.showOnboarding(Utils.shouldShowCryptoOnboarding());
    }

    @Override
    public void onConnectionError(MojoException e) {
        mKeyringService.close();
        mKeyringService = null;
        initKeyringService();
        updateState();
    }

    public void dismiss() {
        if (isShowingPanel()) {
            mBraveWalletPanel.dismiss();
        }
        if (isShowingDialog()) {
            mDAppsDialog.dismiss();
        }
        cleanUp();
    }

    public boolean isShowingPanel() {
        return mBraveWalletPanel != null && mBraveWalletPanel.isShowing();
    }

    public boolean isShowingDialog() {
        return mDAppsDialog != null && mDAppsDialog.isShowing();
    }

    private void updateState() {
        if (mHasStateInitialise) return;
        // remember the dialog has been shown
        mHasStateInitialise = true;
    }

    private void initKeyringService() {
        if (mKeyringService != null) {
            return;
        }

        mKeyringService = KeyringServiceFactory.getInstance().getKeyringService(this);
    }

    private void cleanUp() {
        if (mKeyringService != null) {
            mKeyringService.close();
        }
    }
}
