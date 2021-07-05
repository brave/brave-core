/**
 * Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.vpn;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.chrome.browser.vpn.BraveVpnObserver;

import java.util.ArrayList;
import java.util.List;

@JNINamespace("chrome::android")
public class BraveVpnNativeWorker {
    private long mNativeBraveVpnNativeWorker;
    private static final Object lock = new Object();
    private static BraveVpnNativeWorker instance;

    private List<BraveVpnObserver> mObservers;

    public static BraveVpnNativeWorker getInstance() {
        synchronized (lock) {
            if (instance == null) {
                instance = new BraveVpnNativeWorker();
                instance.init();
            }
        }
        return instance;
    }

    private BraveVpnNativeWorker() {
        mObservers = new ArrayList<BraveVpnObserver>();
    }

    private void init() {
        if (mNativeBraveVpnNativeWorker == 0) {
            nativeInit();
        }
    }

    @Override
    protected void finalize() {
        destroy();
    }

    private void destroy() {
        if (mNativeBraveVpnNativeWorker != 0) {
            nativeDestroy(mNativeBraveVpnNativeWorker);
            mNativeBraveVpnNativeWorker = 0;
        }
    }

    public void addObserver(BraveVpnObserver observer) {
        synchronized (lock) {
            mObservers.add(observer);
        }
    }

    public void removeObserver(BraveVpnObserver observer) {
        synchronized (lock) {
            mObservers.remove(observer);
        }
    }

    @CalledByNative
    private void setNativePtr(long nativePtr) {
        assert mNativeBraveVpnNativeWorker == 0;
        mNativeBraveVpnNativeWorker = nativePtr;
    }

    @CalledByNative
    public void onGetAllServerRegions(String jsonServerRegions, boolean isSuccess) {
        for (BraveVpnObserver observer : mObservers) {
            observer.onGetAllServerRegions(jsonServerRegions, isSuccess);
        }
    }

    @CalledByNative
    public void onGetTimezonesForRegions(String jsonTimezones, boolean isSuccess) {
        for (BraveVpnObserver observer : mObservers) {
            observer.onGetTimezonesForRegions(jsonTimezones, isSuccess);
        }
    }

    @CalledByNative
    public void onGetHostnamesForRegion(String jsonHostnames, boolean isSuccess) {
        for (BraveVpnObserver observer : mObservers) {
            observer.onGetHostnamesForRegion(jsonHostnames, isSuccess);
        }
    }

    @CalledByNative
    public void onGetSubscriberCredential(String subscriberCredential, boolean isSuccess) {
        for (BraveVpnObserver observer : mObservers) {
            observer.onGetSubscriberCredential(subscriberCredential, isSuccess);
        }
    }

    @CalledByNative
    public void onVerifyPurchaseToken(String jsonResponse, boolean isSuccess) {
        for (BraveVpnObserver observer : mObservers) {
            observer.onVerifyPurchaseToken(jsonResponse, isSuccess);
        }
    }

    public void getAllServerRegions() {
        nativeGetAllServerRegions(mNativeBraveVpnNativeWorker);
    }

    public void getTimezonesForRegions() {
        nativeGetTimezonesForRegions(mNativeBraveVpnNativeWorker);
    }

    public void getHostnamesForRegion(String region) {
        nativeGetHostnamesForRegion(mNativeBraveVpnNativeWorker, region);
    }

    public void getSubscriberCredential(
            String productType, String productId, String validationMethod, String purchaseToken) {
        nativeGetSubscriberCredential(mNativeBraveVpnNativeWorker, productType, productId,
                validationMethod, purchaseToken);
    }

    public void verifyPurchaseToken(String purchaseToken, String productId, String productType) {
        nativeVerifyPurchaseToken(
                mNativeBraveVpnNativeWorker, purchaseToken, productId, productType);
    }

    private native void nativeInit();
    private native void nativeDestroy(long nativeBraveVpnNativeWorker);
    private native void nativeGetAllServerRegions(long nativeBraveVpnNativeWorker);
    private native void nativeGetTimezonesForRegions(long nativeBraveVpnNativeWorker);
    private native void nativeGetHostnamesForRegion(long nativeBraveVpnNativeWorker, String region);
    private native void nativeGetSubscriberCredential(long nativeBraveVpnNativeWorker,
            String productType, String productId, String validationMethod, String purchaseToken);
    private native void nativeVerifyPurchaseToken(long nativeBraveVpnNativeWorker,
            String purchaseToken, String productId, String productType);
}
