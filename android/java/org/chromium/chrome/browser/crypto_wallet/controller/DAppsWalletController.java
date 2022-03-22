/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.controller;

import android.content.Context;
import android.content.DialogInterface;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.lifecycle.DefaultLifecycleObserver;
import androidx.lifecycle.LifecycleOwner;

import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.BraveWalletConstants;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.crypto_wallet.JsonRpcServiceFactory;
import org.chromium.chrome.browser.crypto_wallet.KeyringServiceFactory;
import org.chromium.chrome.browser.crypto_wallet.modal.BraveWalletPanel;
import org.chromium.chrome.browser.crypto_wallet.modal.DAppsDialog;
import org.chromium.chrome.browser.crypto_wallet.observers.KeyringServiceObserver;
import org.chromium.chrome.browser.crypto_wallet.util.AccountsPermissionsHelper;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;

public class DAppsWalletController implements ConnectionErrorHandler, KeyringServiceObserver {
    private Context mContext;
    private View mAnchorViewHost;
    private KeyringService mKeyringService;
    protected JsonRpcService mJsonRpcService;
    private boolean mHasStateInitialise;
    private DAppsDialog mDAppsDialog;
    private BraveWalletPanel mBraveWalletPanel;
    private DialogInterface.OnDismissListener mOnDismissListener;
    private final AppCompatActivity mActivity;

    private DialogInterface.OnDismissListener mDialogOrPanelDismissListener = dialog -> {
        if (mOnDismissListener != null) {
            mOnDismissListener.onDismiss(dialog);
        }
        cleanUp();
    };

    public DAppsWalletController(Context mContext, View mAnchorViewHost) {
        this.mContext = mContext;
        this.mAnchorViewHost = mAnchorViewHost;
        this.mActivity = BraveActivity.getChromeTabbedActivity();
    }

    public DAppsWalletController(Context mContext, View mAnchorViewHost,
            DialogInterface.OnDismissListener onDismissListener) {
        this(mContext, mAnchorViewHost);
        this.mOnDismissListener = onDismissListener;
    }

    public void showWalletPanel() {
        InitKeyringService();
        InitJsonRpcService();
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
                        createAndShowWalletPanel();
                    }
                }
            });
        }
    }

    private void createAndShowWalletPanel() {
        mKeyringService.getKeyringInfo(BraveWalletConstants.DEFAULT_KEYRING_ID, keyringInfo -> {
            AccountInfo[] accountInfos = new AccountInfo[0];
            if (keyringInfo != null) {
                accountInfos = keyringInfo.accountInfos;
            }
            mBraveWalletPanel = new BraveWalletPanel(
                    mAnchorViewHost, mDialogOrPanelDismissListener, mJsonRpcService, accountInfos);
            mBraveWalletPanel.showLikePopDownMenu();
            setupLifeCycleUpdater();
        });
    }

    private void setupLifeCycleUpdater() {
        mActivity.getLifecycle().addObserver(defaultLifecycleObserver);
    }

    private void showOnBoardingOrUnlock() {
        mDAppsDialog = DAppsDialog.newInstance(mContext, mDialogOrPanelDismissListener);
        mDAppsDialog.showOnboarding(Utils.shouldShowCryptoOnboarding());
    }

    @Override
    public void onConnectionError(MojoException e) {
        mKeyringService.close();
        mJsonRpcService.close();
        mKeyringService = null;
        mJsonRpcService = null;
        InitKeyringService();
        InitJsonRpcService();
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

    private void InitKeyringService() {
        if (mKeyringService != null) {
            return;
        }

        mKeyringService =
                KeyringServiceFactory.getInstance(Utils.getProfile(false)).getKeyringService(this);
    }

    private void InitJsonRpcService() {
        if (mJsonRpcService != null) {
            return;
        }

        mJsonRpcService = JsonRpcServiceFactory.getInstance().getJsonRpcService(this);
    }

    private void cleanUp() {
        if (mKeyringService != null) {
            mKeyringService.close();
        }
        if (mJsonRpcService != null) {
            mJsonRpcService.close();
        }

        if (mActivity != null) {
            mActivity.getLifecycle().removeObserver(defaultLifecycleObserver);
        }
    }

    private final DefaultLifecycleObserver defaultLifecycleObserver =
            new DefaultLifecycleObserver() {
                @Override
                public void onResume(@NonNull LifecycleOwner owner) {
                    if (mBraveWalletPanel != null) {
                        mBraveWalletPanel.resume();
                    }
                }
            };
}
