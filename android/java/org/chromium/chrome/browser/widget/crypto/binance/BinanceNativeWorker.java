/**
 * Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.widget.crypto.binance;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
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
            nativeInit();
        }
    }

    @Override
    protected void finalize() {
        Destroy();
    }

    private void Destroy() {
        if (mNativeBinanceNativeWorker != 0) {
            nativeDestroy(mNativeBinanceNativeWorker);
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
            nativeGetAccessToken(mNativeBinanceNativeWorker);
        }
    }

    public boolean IsSupportedRegion() {
        synchronized(lock) {
            return nativeIsSupportedRegion(mNativeBinanceNativeWorker);
        }
    }

    public String getLocaleForURL() {
        return nativeGetLocaleForURL(mNativeBinanceNativeWorker);
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
        return nativeGetOAuthClientUrl(mNativeBinanceNativeWorker);
    }

    public void setAuthToken(String authToken) {
        nativeSetAuthToken(mNativeBinanceNativeWorker, authToken);
    }

    public void getAccountBalances() {
        nativeGetAccountBalances(mNativeBinanceNativeWorker);
    }

    public void getConvertQuote(String from, String to, String amount) {
        nativeGetConvertQuote(mNativeBinanceNativeWorker, from, to, amount);
    }

    public void getCoinNetworks() {
        nativeGetCoinNetworks(mNativeBinanceNativeWorker);
    }

    public void getDepositInfo(String symbol, String tickerNetwork) {
        nativeGetDepositInfo(mNativeBinanceNativeWorker, symbol, tickerNetwork);
    }

    public void confirmConvert(String quoteId) {
        nativeConfirmConvert(mNativeBinanceNativeWorker, quoteId);
    }

    public void getConvertAssets() {
        nativeGetConvertAssets(mNativeBinanceNativeWorker);
    }

    public void revokeToken() {
        nativeRevokeToken(mNativeBinanceNativeWorker);
    }

    private native void nativeInit();
    private native void nativeDestroy(long nativeBinanceNativeWorker);
    private native String nativeGetOAuthClientUrl(long nativeBinanceNativeWorker);
    private native void nativeGetAccessToken(long nativeBinanceNativeWorker);
    private native boolean nativeIsSupportedRegion(long nativeBinanceNativeWorker);
    private native String nativeGetLocaleForURL(long nativeBinanceNativeWorker);
    private native void nativeSetAuthToken(long nativeBinanceNativeWorker, String authToken);
    private native void nativeGetAccountBalances(long nativeBinanceNativeWorker);
    private native void nativeGetConvertQuote(
            long nativeBinanceNativeWorker, String from, String to, String amount);
    private native void nativeGetCoinNetworks(long nativeBinanceNativeWorker);
    private native void nativeGetDepositInfo(
            long nativeBinanceNativeWorker, String symbol, String tickerNetwork);
    private native void nativeConfirmConvert(long nativeBinanceNativeWorker, String quoteId);
    private native void nativeGetConvertAssets(long nativeBinanceNativeWorker);
    private native void nativeRevokeToken(long nativeBinanceNativeWorker);
}