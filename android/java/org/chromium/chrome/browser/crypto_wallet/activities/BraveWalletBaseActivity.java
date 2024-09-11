/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.activities;

import android.view.MenuItem;

import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.brave_wallet.mojom.AssetRatioService;
import org.chromium.brave_wallet.mojom.BlockchainRegistry;
import org.chromium.brave_wallet.mojom.BraveWalletP3a;
import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.EthTxManagerProxy;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.brave_wallet.mojom.SolanaTxManagerProxy;
import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.brave_wallet.mojom.TxService;
import org.chromium.chrome.browser.crypto_wallet.AssetRatioServiceFactory;
import org.chromium.chrome.browser.crypto_wallet.BlockchainRegistryFactory;
import org.chromium.chrome.browser.crypto_wallet.BraveWalletServiceFactory;
import org.chromium.chrome.browser.crypto_wallet.observers.KeyringServiceObserverImpl;
import org.chromium.chrome.browser.crypto_wallet.observers.KeyringServiceObserverImpl.KeyringServiceObserverImplDelegate;
import org.chromium.chrome.browser.crypto_wallet.observers.TxServiceObserverImpl;
import org.chromium.chrome.browser.crypto_wallet.observers.TxServiceObserverImpl.TxServiceObserverImplDelegate;
import org.chromium.chrome.browser.init.ActivityProfileProvider;
import org.chromium.chrome.browser.init.AsyncInitializationActivity;
import org.chromium.chrome.browser.profiles.ProfileProvider;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;

public abstract class BraveWalletBaseActivity extends AsyncInitializationActivity
        implements ConnectionErrorHandler,
                KeyringServiceObserverImplDelegate,
                TxServiceObserverImplDelegate {
    protected KeyringService mKeyringService;
    protected BlockchainRegistry mBlockchainRegistry;
    protected JsonRpcService mJsonRpcService;
    protected TxService mTxService;
    protected EthTxManagerProxy mEthTxManagerProxy;
    protected SolanaTxManagerProxy mSolanaTxManagerProxy;
    protected AssetRatioService mAssetRatioService;
    protected BraveWalletP3a mBraveWalletP3A;
    protected BraveWalletService mBraveWalletService;
    private KeyringServiceObserverImpl mKeyringServiceObserver;
    private TxServiceObserverImpl mTxServiceObserver;

    @Override
    public void onUserInteraction() {
        if (mKeyringService == null) {
            return;
        }
        mKeyringService.notifyUserInteraction();
    }

    @Override
    public void onConnectionError(MojoException e) {
        if (mKeyringServiceObserver != null) {
            mKeyringServiceObserver.close();
            mKeyringServiceObserver = null;
        }
        if (mTxServiceObserver != null) {
            mTxServiceObserver.close();
            mTxServiceObserver.destroy();
            mTxServiceObserver = null;
        }
        if (mKeyringService != null) mKeyringService.close();
        if (mAssetRatioService != null) mAssetRatioService.close();
        if (mBlockchainRegistry != null) mBlockchainRegistry.close();
        if (mJsonRpcService != null) mJsonRpcService.close();
        if (mTxService != null) mTxService.close();
        if (mEthTxManagerProxy != null) mEthTxManagerProxy.close();
        if (mSolanaTxManagerProxy != null) mSolanaTxManagerProxy.close();
        if (mBraveWalletP3A != null) mBraveWalletP3A.close();
        if (mBraveWalletService != null) mBraveWalletService.close();

        mKeyringService = null;
        mBlockchainRegistry = null;
        mJsonRpcService = null;
        mTxService = null;
        mEthTxManagerProxy = null;
        mSolanaTxManagerProxy = null;
        mAssetRatioService = null;
        mBraveWalletP3A = null;
        mBraveWalletService = null;
        InitKeyringService();
        InitBlockchainRegistry();
        InitJsonRpcService();
        InitTxService();
        InitEthTxManagerProxy();
        InitSolanaTxManagerProxy();
        InitAssetRatioService();
        InitBraveWalletP3A();
        InitBraveWalletService();
    }

    protected void InitTxService() {
        if (mTxService != null) {
            return;
        }

        mTxService = BraveWalletServiceFactory.getInstance().getTxService(this);
        mTxServiceObserver = new TxServiceObserverImpl(this);
        mTxService.addObserver(mTxServiceObserver);
    }

    protected void InitEthTxManagerProxy() {
        if (mEthTxManagerProxy != null) {
            return;
        }

        mEthTxManagerProxy = BraveWalletServiceFactory.getInstance().getEthTxManagerProxy(this);
    }

    protected void InitSolanaTxManagerProxy() {
        if (mSolanaTxManagerProxy != null) {
            return;
        }

        mSolanaTxManagerProxy =
                BraveWalletServiceFactory.getInstance().getSolanaTxManagerProxy(this);
    }

    protected void InitKeyringService() {
        if (mKeyringService != null) {
            return;
        }

        mKeyringService = BraveWalletServiceFactory.getInstance().getKeyringService(this);
        mKeyringServiceObserver = new KeyringServiceObserverImpl(this);
        mKeyringService.addObserver(mKeyringServiceObserver);
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

        mJsonRpcService = BraveWalletServiceFactory.getInstance().getJsonRpcService(this);
    }

    protected void InitAssetRatioService() {
        if (mAssetRatioService != null) {
            return;
        }

        mAssetRatioService = AssetRatioServiceFactory.getInstance().getAssetRatioService(this);
    }

    protected void InitBraveWalletP3A() {
        if (mBraveWalletP3A != null) {
            return;
        }

        mBraveWalletP3A = BraveWalletServiceFactory.getInstance().getBraveWalletP3A(this);
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

    public TxService getTxService() {
        return mTxService;
    }

    public EthTxManagerProxy getEthTxManagerProxy() {
        return mEthTxManagerProxy;
    }

    public SolanaTxManagerProxy getSolanaTxManagerProxy() {
        return mSolanaTxManagerProxy;
    }

    public AssetRatioService getAssetRatioService() {
        return mAssetRatioService;
    }

    public BraveWalletService getBraveWalletService() {
        return mBraveWalletService;
    }

    public BraveWalletP3a getBraveWalletP3A() {
        return mBraveWalletP3A;
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
        InitTxService();
        InitEthTxManagerProxy();
        InitSolanaTxManagerProxy();
        InitAssetRatioService();
        InitBraveWalletP3A();
        InitBraveWalletService();
    }

    @Override
    public void onDestroy() {
        if (mKeyringServiceObserver != null) {
            mKeyringServiceObserver.close();
        }
        if (mTxServiceObserver != null) {
            mTxServiceObserver.close();
            mTxServiceObserver.destroy();
        }
        if (mKeyringService != null) mKeyringService.close();
        if (mAssetRatioService != null) mAssetRatioService.close();
        if (mBlockchainRegistry != null) mBlockchainRegistry.close();
        if (mJsonRpcService != null) mJsonRpcService.close();
        if (mTxService != null) mTxService.close();
        if (mEthTxManagerProxy != null) mEthTxManagerProxy.close();
        if (mSolanaTxManagerProxy != null) mSolanaTxManagerProxy.close();
        if (mBraveWalletP3A != null) mBraveWalletP3A.close();
        if (mBraveWalletService != null) mBraveWalletService.close();
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

    @Override
    public void backedUp() {}

    @Override
    public void onNewUnapprovedTx(TransactionInfo txInfo) {}

    @Override
    public void onUnapprovedTxUpdated(TransactionInfo txInfo) {}

    @Override
    public void onTransactionStatusChanged(TransactionInfo txInfo) {}

    @Override
    protected OneshotSupplier<ProfileProvider> createProfileProvider() {
        return new ActivityProfileProvider(getLifecycleDispatcher());
    }
}
