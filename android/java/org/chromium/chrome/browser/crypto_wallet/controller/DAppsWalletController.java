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
import org.chromium.brave_wallet.mojom.AssetRatioService;
import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.crypto_wallet.AssetRatioServiceFactory;
import org.chromium.chrome.browser.crypto_wallet.BraveWalletServiceFactory;
import org.chromium.chrome.browser.crypto_wallet.JsonRpcServiceFactory;
import org.chromium.chrome.browser.crypto_wallet.KeyringServiceFactory;
import org.chromium.chrome.browser.crypto_wallet.modal.BraveWalletPanel;
import org.chromium.chrome.browser.crypto_wallet.modal.DAppsDialog;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.toolbar.bottom.BottomToolbarConfiguration;
import org.chromium.chrome.browser.util.ConfigurationUtils;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;

public class DAppsWalletController
        implements ConnectionErrorHandler, BraveWalletPanel.BraveWalletPanelServices {
    private Context mContext;
    private View mAnchorViewHost;
    private AssetRatioService mAssetRatioService;
    private KeyringService mKeyringService;
    private BraveWalletService mBraveWalletService;
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
        InitAssetRatioService();
        InitKeyringService();
        InitJsonRpcService();
        InitBraveWalletService();
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
        mBraveWalletPanel =
                new BraveWalletPanel(mAnchorViewHost, mDialogOrPanelDismissListener, this);
        mBraveWalletPanel.showLikePopDownMenu();
        setupLifeCycleUpdater();
    }

    private void setupLifeCycleUpdater() {
        mActivity.getLifecycle().addObserver(defaultLifecycleObserver);
    }

    private void showOnBoardingOrUnlock() {
        int dialogStyle = DAppsDialog.DAppsDialogStyle.BOTTOM;
        if (shouldShowNotificationAtTop(mActivity)) {
            dialogStyle = DAppsDialog.DAppsDialogStyle.TOP;
        }
        mDAppsDialog =
                DAppsDialog.newInstance(mContext, mDialogOrPanelDismissListener, dialogStyle);
        mDAppsDialog.showOnboarding(Utils.shouldShowCryptoOnboarding());
    }

    @Override
    public AssetRatioService getAssetRatioService() {
        assert mAssetRatioService != null;
        return mAssetRatioService;
    }

    @Override
    public BraveWalletService getBraveWalletService() {
        assert mBraveWalletService != null;
        return mBraveWalletService;
    }

    @Override
    public KeyringService getKeyringService() {
        assert mKeyringService != null;
        return mKeyringService;
    }

    @Override
    public JsonRpcService getJsonRpcService() {
        assert mJsonRpcService != null;
        return mJsonRpcService;
    }

    @Override
    public void onConnectionError(MojoException e) {
        if (mKeyringService != null) {
            mKeyringService.close();
            mKeyringService = null;
        }
        if (mJsonRpcService != null) {
            mJsonRpcService.close();
            mJsonRpcService = null;
        }
        if (mBraveWalletService != null) {
            mBraveWalletService.close();
            mBraveWalletService = null;
        }
        if (mAssetRatioService != null) {
            mAssetRatioService.close();
            mAssetRatioService = null;
        }
        InitAssetRatioService();
        InitKeyringService();
        InitJsonRpcService();
        InitBraveWalletService();
        updateState();
    }

    public void dismiss() {
        if (isShowingPanel()) {
            mBraveWalletPanel.dismiss();
        }
        if (isShowingDialog()) {
            mDAppsDialog.dismiss();
        }
        mBraveWalletPanel = null;
        mDAppsDialog = null;
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
        mKeyringService = KeyringServiceFactory.getInstance().getKeyringService(this);
    }

    private void InitJsonRpcService() {
        if (mJsonRpcService != null) {
            return;
        }
        mJsonRpcService = JsonRpcServiceFactory.getInstance().getJsonRpcService(this);
    }

    private void InitBraveWalletService() {
        if (mBraveWalletService != null) {
            return;
        }
        mBraveWalletService = BraveWalletServiceFactory.getInstance().getBraveWalletService(this);
    }

    private void InitAssetRatioService() {
        if (mAssetRatioService != null) {
            return;
        }
        mAssetRatioService = AssetRatioServiceFactory.getInstance().getAssetRatioService(this);
    }

    private void cleanUp() {
        if (mKeyringService != null) {
            mKeyringService.close();
            mKeyringService = null;
        }
        if (mJsonRpcService != null) {
            mJsonRpcService.close();
            mJsonRpcService = null;
        }
        if (mBraveWalletService != null) {
            mBraveWalletService.close();
            mBraveWalletService = null;
        }
        if (mAssetRatioService != null) {
            mAssetRatioService.close();
            mAssetRatioService = null;
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

                @Override
                public void onPause(@NonNull LifecycleOwner owner) {
                    if (mBraveWalletPanel != null) {
                        mBraveWalletPanel.pause();
                    }
                }
            };

    private boolean shouldShowNotificationAtTop(Context context) {
        return ConfigurationUtils.isTablet(context)
                || !BottomToolbarConfiguration.isBottomToolbarEnabled();
    }
}
