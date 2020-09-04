/** Copyright (c) 2020 The Brave Authors. All rights reserved.
  * This Source Code Form is subject to the terms of the Mozilla Public
  * License, v. 2.0. If a copy of the MPL was not distributed with this file,
  * You can obtain one at http://mozilla.org/MPL/2.0/.
  */

package org.chromium.chrome.browser.widget.crypto.binance;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

@JNINamespace("chrome::android")
public class BinanceNativeWorker {
    private long mNativeBinanceNativeWorker;
    private static final Object lock = new Object();
    private static BinanceNativeWorker instance;

    public static  BinanceNativeWorker getInstance() {
        synchronized (lock) {
            if (instance == null) {
                instance = new BinanceNativeWorker();
                instance.Init();
            }
        }
        return instance;
    }

    private BinanceNativeWorker() {
        // mObservers = new ArrayList<BraveRewardsObserver>();
        // mFrontTabPublisherObservers = new ArrayList<PublisherObserver>();
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

    @CalledByNative
    private void setNativePtr(long nativePtr) {
        assert mNativeBinanceNativeWorker == 0;
        mNativeBinanceNativeWorker = nativePtr;
    }

    @CalledByNative
    public void OnGetAccessToken(boolean isSuccess) {
    }

    @CalledByNative
    public void OnGetAccountBalances(String jsonBalances, boolean isSuccess) {
    }

    @CalledByNative
    public void OnGetConvertQuote(String quoteId, String quotePrice, String totalFee, String totalAmount) {
    }

    @CalledByNative
    public void OnGetCoinNetworks(String jsonNetworks) {
    }

    @CalledByNative
    public void OnGetDepositInfo(String depositAddress, String depositeTag, boolean isSuccess) {
    }

    @CalledByNative
    public void OnConfirmConvert(boolean isSuccess, String message) {
    }

    @CalledByNative
    public void OnGetConvertAssets(String jsonAssets) {
    }

    @CalledByNative
    public void OnRevokeToken(boolean isSuccess) {
    }

    public String getOAuthClientUrl() {
        return nativeGetOAuthClientUrl(mNativeBinanceNativeWorker);
    }

    private native void nativeInit();
    private native void nativeDestroy(long nativeBinanceNativeWorker);
    private native String nativeGetOAuthClientUrl(long nativeBinanceNativeWorker);
}