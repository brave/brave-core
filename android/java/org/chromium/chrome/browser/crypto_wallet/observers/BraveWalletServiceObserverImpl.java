/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.observers;

import org.chromium.brave_wallet.mojom.BlockchainToken;
import org.chromium.brave_wallet.mojom.BraveWalletServiceObserver;
import org.chromium.brave_wallet.mojom.OriginInfo;
import org.chromium.mojo.system.MojoException;

import java.lang.ref.WeakReference;

public class BraveWalletServiceObserverImpl implements BraveWalletServiceObserver {
    public interface BraveWalletServiceObserverImplDelegate {
        default void onActiveOriginChanged(OriginInfo originInfo) {}

        default void onDefaultEthereumWalletChanged(int wallet) {}

        default void onDefaultSolanaWalletChanged(int wallet) {}

        default void onDefaultBaseCurrencyChanged(String currency) {}

        default void onDefaultBaseCryptocurrencyChanged(String cryptocurrency) {}

        default void onNetworkListChanged() {}

        default void onDiscoverAssetsStarted() {}

        default void onDiscoverAssetsCompleted(BlockchainToken[] discoveredAssets) {}

        default void onResetWallet() {}
    }

    private WeakReference<BraveWalletServiceObserverImplDelegate> mDelegate;

    public BraveWalletServiceObserverImpl(BraveWalletServiceObserverImplDelegate delegate) {
        mDelegate = new WeakReference<>(delegate);
    }

    @Override
    public void onActiveOriginChanged(OriginInfo originInfo) {
        if (isActive()) getRef().onActiveOriginChanged(originInfo);
    }

    @Override
    public void onDefaultEthereumWalletChanged(int wallet) {
        if (isActive()) getRef().onDefaultEthereumWalletChanged(wallet);
    }

    @Override
    public void onDefaultSolanaWalletChanged(int wallet) {
        if (isActive()) getRef().onDefaultSolanaWalletChanged(wallet);
    }

    @Override
    public void onDefaultBaseCurrencyChanged(String currency) {
        if (isActive()) getRef().onDefaultBaseCurrencyChanged(currency);
    }

    @Override
    public void onDefaultBaseCryptocurrencyChanged(String cryptocurrency) {
        if (isActive()) getRef().onDefaultBaseCryptocurrencyChanged(cryptocurrency);
    }

    @Override
    public void onNetworkListChanged() {
        if (isActive()) getRef().onNetworkListChanged();
    }

    @Override
    public void onDiscoverAssetsStarted() {}

    @Override
    public void onDiscoverAssetsCompleted(BlockchainToken[] discoveredAssets) {
        if (isActive()) getRef().onDiscoverAssetsCompleted(discoveredAssets);
    }

    @Override
    public void onResetWallet() {
        if (isActive()) getRef().onResetWallet();
    }

    @Override
    public void close() {
        mDelegate.clear();
        mDelegate = null;
    }

    @Override
    public void onConnectionError(MojoException e) {}

    public void destroy() {
        close();
    }

    private BraveWalletServiceObserverImplDelegate getRef() {
        return mDelegate.get();
    }

    private boolean isActive() {
        return mDelegate != null && mDelegate.get() != null;
    }
}
