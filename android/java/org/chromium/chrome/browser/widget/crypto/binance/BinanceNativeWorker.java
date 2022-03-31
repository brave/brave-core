/**
 * Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.widget.crypto.binance;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.chrome.browser.widget.crypto.binance.BinanceObserver;

import java.util.ArrayList;
import java.util.List;

@JNINamespace("chrome::android")
public class BinanceNativeWorker {
    private long mNativeBinanceNativeWorker;
    private static final Object lock = new Object();
    private static BinanceNativeWorker instance;

    private List<BinanceObserver> mObservers;

    public static BinanceNativeWorker getInstance() {
        synchronized (lock) {
            if (instance == null) {
                instance = new BinanceNativeWorker();
                instance.Init();
            }
        }
        return instance;
    }

    private BinanceNativeWorker() {
        mObservers = new ArrayList<BinanceObserver>();
    }

    private void Init() {
        if (mNativeBinanceNativeWorker == 0) {
            BinanceNativeWorkerJni.get().init(this);
        }
    }

    @Override
    protected void finalize() {
        Destroy();
    }

    private void Destroy() {
        if (mNativeBinanceNativeWorker != 0) {
            BinanceNativeWorkerJni.get().destroy(mNativeBinanceNativeWorker, this);
            mNativeBinanceNativeWorker = 0;
        }
    }

    public void AddObserver(BinanceObserver observer) {
        synchronized (lock) {
            mObservers.add(observer);
        }
    }

    public void RemoveObserver(BinanceObserver observer) {
        synchronized (lock) {
            mObservers.remove(observer);
        }
    }

    public void getAccessToken() {
        synchronized (lock) {
            BinanceNativeWorkerJni.get().getAccessToken(mNativeBinanceNativeWorker);
        }
    }

    public boolean IsSupportedRegion() {
        synchronized (lock) {
            return BinanceNativeWorkerJni.get().isSupportedRegion(mNativeBinanceNativeWorker);
        }
    }

    public String getLocaleForURL() {
        return BinanceNativeWorkerJni.get().getLocaleForURL(mNativeBinanceNativeWorker);
    }

    @CalledByNative
    private void setNativePtr(long nativePtr) {
        assert mNativeBinanceNativeWorker == 0;
        mNativeBinanceNativeWorker = nativePtr;
    }

    @CalledByNative
    public void OnGetAccessToken(boolean isSuccess) {
        for (BinanceObserver observer : mObservers) {
            observer.OnGetAccessToken(isSuccess);
        }
    }

    @CalledByNative
    public void OnGetAccountBalances(String jsonBalances, boolean isSuccess) {
        for (BinanceObserver observer : mObservers) {
            observer.OnGetAccountBalances(jsonBalances, isSuccess);
        }
    }

    @CalledByNative
    public void OnGetConvertQuote(
            String quoteId, String quotePrice, String totalFee, String totalAmount) {
        for (BinanceObserver observer : mObservers) {
            observer.OnGetConvertQuote(quoteId, quotePrice, totalFee, totalAmount);
        }
    }

    @CalledByNative
    public void OnGetCoinNetworks(String jsonNetworks) {
        for (BinanceObserver observer : mObservers) {
            observer.OnGetCoinNetworks(jsonNetworks);
        }
    }

    @CalledByNative
    public void OnGetDepositInfo(String depositAddress, String depositTag, boolean isSuccess) {
        for (BinanceObserver observer : mObservers) {
            observer.OnGetDepositInfo(depositAddress, depositTag, isSuccess);
        }
    }

    @CalledByNative
    public void OnConfirmConvert(boolean isSuccess, String message) {
        for (BinanceObserver observer : mObservers) {
            observer.OnConfirmConvert(isSuccess, message);
        }
    }

    @CalledByNative
    public void OnGetConvertAssets(String jsonAssets) {
        for (BinanceObserver observer : mObservers) {
            observer.OnGetConvertAssets(jsonAssets);
        }
    }

    @CalledByNative
    public void OnRevokeToken(boolean isSuccess) {
        for (BinanceObserver observer : mObservers) {
            observer.OnRevokeToken(isSuccess);
        }
    }

    public String getOAuthClientUrl() {
        return BinanceNativeWorkerJni.get().getOAuthClientUrl(mNativeBinanceNativeWorker);
    }

    public void setAuthToken(String authToken) {
        BinanceNativeWorkerJni.get().setAuthToken(mNativeBinanceNativeWorker, authToken);
    }

    public void getAccountBalances() {
        BinanceNativeWorkerJni.get().getAccountBalances(mNativeBinanceNativeWorker);
    }

    public void getConvertQuote(String from, String to, String amount) {
        BinanceNativeWorkerJni.get().getConvertQuote(mNativeBinanceNativeWorker, from, to, amount);
    }

    public void getCoinNetworks() {
        BinanceNativeWorkerJni.get().getCoinNetworks(mNativeBinanceNativeWorker);
    }

    public void getDepositInfo(String symbol, String tickerNetwork) {
        BinanceNativeWorkerJni.get().getDepositInfo(
                mNativeBinanceNativeWorker, symbol, tickerNetwork);
    }

    public void confirmConvert(String quoteId) {
        BinanceNativeWorkerJni.get().confirmConvert(mNativeBinanceNativeWorker, quoteId);
    }

    public void getConvertAssets() {
        BinanceNativeWorkerJni.get().getConvertAssets(mNativeBinanceNativeWorker);
    }

    public void revokeToken() {
        BinanceNativeWorkerJni.get().revokeToken(mNativeBinanceNativeWorker);
    }

    @NativeMethods
    interface Natives {
        void init(BinanceNativeWorker caller);
        void destroy(long nativeBinanceNativeWorker, BinanceNativeWorker caller);
        String getOAuthClientUrl(long nativeBinanceNativeWorker);
        void getAccessToken(long nativeBinanceNativeWorker);
        boolean isSupportedRegion(long nativeBinanceNativeWorker);
        String getLocaleForURL(long nativeBinanceNativeWorker);
        void setAuthToken(long nativeBinanceNativeWorker, String authToken);
        void getAccountBalances(long nativeBinanceNativeWorker);
        void getConvertQuote(long nativeBinanceNativeWorker, String from, String to, String amount);
        void getCoinNetworks(long nativeBinanceNativeWorker);
        void getDepositInfo(long nativeBinanceNativeWorker, String symbol, String tickerNetwork);
        void confirmConvert(long nativeBinanceNativeWorker, String quoteId);
        void getConvertAssets(long nativeBinanceNativeWorker);
        void revokeToken(long nativeBinanceNativeWorker);
    }
}
