/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
 
 package org.chromium.chrome.browser.crypto_wallet.controller;

import android.content.Context;
import android.view.View;

import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.chrome.browser.crypto_wallet.KeyringServiceFactory;
import org.chromium.chrome.browser.crypto_wallet.modal.BraveWalletPanel;
import org.chromium.chrome.browser.crypto_wallet.modal.DappsDialog;
import org.chromium.chrome.browser.crypto_wallet.observers.KeyringServiceObserver;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;

public class DAppsWalletController implements ConnectionErrorHandler, KeyringServiceObserver {
    private Context mContext;
    private View mAnchorViewHost;
    private KeyringService mKeyringService;
    private boolean mHasStateInitialise;
    private DappsDialog dappsDialog;
    private BraveWalletPanel braveWalletPanel;

    public DAppsWalletController(Context mContext, View mAnchorViewHost) {
        this.mContext = mContext;
        this.mAnchorViewHost = mAnchorViewHost;
    }

    public void showWalletPanel() {
        initKeyringService();
        if (Utils.shouldShowCryptoOnboarding()) {
            showOnBoarding();
        } else {
            mKeyringService.isLocked(isLocked -> {
                if (isLocked) {
                    showOnBoarding();
                } else {
                    boolean isFoundPendingDAppsTx = false;
                    // TODO: check if pending dapps transaction are available and implement an
                    // action accrodingly
                    if (!isFoundPendingDAppsTx) {
                        braveWalletPanel = new BraveWalletPanel(mAnchorViewHost);
                        braveWalletPanel.showLikePopDownMenu();
                    }
                }
            });
        }
    }

    private void showOnBoarding() {
        dappsDialog = DappsDialog.newInstance(mContext);
        dappsDialog.showOnboarding(Utils.shouldShowCryptoOnboarding());
    }

    @Override
    public void onConnectionError(MojoException e) {
        mKeyringService.close();
        mKeyringService = null;
        initKeyringService();
        updateState();
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
}
