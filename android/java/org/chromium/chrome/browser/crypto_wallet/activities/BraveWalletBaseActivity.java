/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.activities;

import android.view.MenuItem;

import org.chromium.brave_wallet.mojom.AssetRatioService;
import org.chromium.brave_wallet.mojom.BlockchainRegistry;
import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.EthTxService;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.chrome.browser.crypto_wallet.AssetRatioServiceFactory;
import org.chromium.chrome.browser.crypto_wallet.BlockchainRegistryFactory;
import org.chromium.chrome.browser.crypto_wallet.BraveWalletServiceFactory;
import org.chromium.chrome.browser.crypto_wallet.EthTxServiceFactory;
import org.chromium.chrome.browser.crypto_wallet.JsonRpcServiceFactory;
import org.chromium.chrome.browser.crypto_wallet.KeyringServiceFactory;
import org.chromium.chrome.browser.crypto_wallet.observers.EthTxServiceObserver;
import org.chromium.chrome.browser.crypto_wallet.observers.KeyringServiceObserver;
import org.chromium.chrome.browser.init.AsyncInitializationActivity;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;

public abstract class BraveWalletBaseActivity extends AsyncInitializationActivity
        implements ConnectionErrorHandler, EthTxServiceObserver, KeyringServiceObserver {
    protected KeyringService mKeyringService;
    protected BlockchainRegistry mBlockchainRegistry;
    protected JsonRpcService mJsonRpcService;
    protected EthTxService mEthTxService;
    protected AssetRatioService mAssetRatioService;
    protected BraveWalletService mBraveWalletService;

    @Override
    public void onUserInteraction() {
        if (mKeyringService == null) {
            return;
        }
        mKeyringService.notifyUserInteraction();
    }

    @Override
    public void onConnectionError(MojoException e) {
        mKeyringService.close();
        mAssetRatioService.close();
        mBlockchainRegistry.close();
        mJsonRpcService.close();
        mEthTxService.close();
        mBraveWalletService.close();

        mKeyringService = null;
        mBlockchainRegistry = null;
        mJsonRpcService = null;
        mEthTxService = null;
        mAssetRatioService = null;
        mBraveWalletService = null;
        InitKeyringService();
        InitBlockchainRegistry();
        InitJsonRpcService();
        InitEthTxService();
        InitAssetRatioService();
        InitBraveWalletService();
    }

    protected void InitEthTxService() {
        if (mEthTxService != null) {
            return;
        }

        mEthTxService = EthTxServiceFactory.getInstance().getEthTxService(this);
        mEthTxService.addObserver(this);
    }

    protected void InitKeyringService() {
        if (mKeyringService != null) {
            return;
        }

        mKeyringService = KeyringServiceFactory.getInstance().getKeyringService(this);
        mKeyringService.addObserver(this);
    }

    protected void InitBlockchainRegistry() {
        if (mBlockchainRegistry != null) {
            return;
        }

        mBlockchainRegistry = BlockchainRegistryFactory.getInstance().getBlockchainRegistry(this);
    }

    protected void InitJsonRpcService() {
        if (mJsonRpcService != null) {
            return;
        }

        mJsonRpcService = JsonRpcServiceFactory.getInstance().getJsonRpcService(this);
    }

    protected void InitAssetRatioService() {
        if (mAssetRatioService != null) {
            return;
        }

        mAssetRatioService = AssetRatioServiceFactory.getInstance().getAssetRatioService(this);
    }

    protected void InitBraveWalletService() {
        if (mBraveWalletService != null) {
            return;
        }

        mBraveWalletService = BraveWalletServiceFactory.getInstance().getBraveWalletService(this);
    }

    public KeyringService getKeyringService() {
        return mKeyringService;
    }

    public BlockchainRegistry getBlockchainRegistry() {
        return mBlockchainRegistry;
    }

    public JsonRpcService getJsonRpcService() {
        return mJsonRpcService;
    }

    public EthTxService getEthTxService() {
        return mEthTxService;
    }

    public AssetRatioService getAssetRatioService() {
        return mAssetRatioService;
    }

    public BraveWalletService getBraveWalletService() {
        return mBraveWalletService;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case android.R.id.home:
                finish();
                return true;
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    public void finishNativeInitialization() {
        super.finishNativeInitialization();
        InitKeyringService();
        InitBlockchainRegistry();
        InitJsonRpcService();
        InitEthTxService();
        InitAssetRatioService();
        InitBraveWalletService();
    }

    @Override
    public void onDestroy() {
        mKeyringService.close();
        mAssetRatioService.close();
        mBlockchainRegistry.close();
        mJsonRpcService.close();
        mEthTxService.close();
        mBraveWalletService.close();
        super.onDestroy();
    }

    @Override
    public boolean shouldStartGpuProcess() {
        return true;
    }

    @Override
    public void locked() {
        finish();
    }
}
