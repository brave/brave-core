/**
 * Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.vpn;

import org.chromium.base.Log;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.chrome.browser.vpn.VpnObserver;

import java.util.ArrayList;
import java.util.List;

@JNINamespace("chrome::android")
public class VpnNativeWorker {
    private long mNativeVpnNativeWorker;
    private static final Object lock = new Object();
    private static VpnNativeWorker instance;

    private List<VpnObserver> mObservers;

    public static VpnNativeWorker getInstance() {
        synchronized (lock) {
            if (instance == null) {
                instance = new VpnNativeWorker();
                instance.init();
            }
        }
        return instance;
    }

    private VpnNativeWorker() {
        mObservers = new ArrayList<VpnObserver>();
    }

    private void init() {
        if (mNativeVpnNativeWorker == 0) {
            nativeInit();
        }
    }

    @Override
    protected void finalize() {
        destroy();
    }

    private void destroy() {
        if (mNativeVpnNativeWorker != 0) {
            nativeDestroy(mNativeVpnNativeWorker);
            mNativeVpnNativeWorker = 0;
        }
    }

    public void addObserver(VpnObserver observer) {
        synchronized (lock) {
            mObservers.add(observer);
        }
    }

    public void removeObserver(VpnObserver observer) {
        synchronized (lock) {
            mObservers.remove(observer);
        }
    }

    @CalledByNative
    private void setNativePtr(long nativePtr) {
        assert mNativeVpnNativeWorker == 0;
        mNativeVpnNativeWorker = nativePtr;
    }

    @CalledByNative
    public void onGetAllServerRegions(String jsonServerRegions, boolean isSuccess) {
        for (VpnObserver observer : mObservers) {
            observer.onGetAllServerRegions(jsonServerRegions, isSuccess);
        }
    }

    @CalledByNative
    public void onGetTimezonesForRegions(String jsonTimezones, boolean isSuccess) {
        for (VpnObserver observer : mObservers) {
            observer.onGetTimezonesForRegions(jsonTimezones, isSuccess);
        }
    }

    @CalledByNative
    public void onGetHostnamesForRegion(String jsonHostnames, boolean isSuccess) {
        for (VpnObserver observer : mObservers) {
            observer.onGetHostnamesForRegion(jsonHostnames, isSuccess);
        }
    }

    @CalledByNative
    public void onGetSubscriberCredential(String subscriberCredential, boolean isSuccess) {
        for (VpnObserver observer : mObservers) {
            observer.onGetSubscriberCredential(subscriberCredential, isSuccess);
        }
    }

    @CalledByNative
    public void onVerifyPurchaseToken(String jsonResponse, boolean isSuccess) {
        for (VpnObserver observer : mObservers) {
            observer.onVerifyPurchaseToken(jsonResponse, isSuccess);
        }
    }

    public void getAllServerRegions() {
        nativeGetAllServerRegions(mNativeVpnNativeWorker);
    }

    public void getTimezonesForRegions() {
        nativeGetTimezonesForRegions(mNativeVpnNativeWorker);
    }

    public void getHostnamesForRegion(String region) {
        nativeGetHostnamesForRegion(mNativeVpnNativeWorker, region);
    }

    public void getSubscriberCredential(
            String productType, String productId, String validationMethod, String purchaseToken) {
        nativeGetSubscriberCredential(
                mNativeVpnNativeWorker, productType, productId, validationMethod, purchaseToken);
    }

    public void verifyPurchaseToken(String purchaseToken, String productId, String productType) {
        nativeVerifyPurchaseToken(mNativeVpnNativeWorker, purchaseToken, productId, productType);
    }

    private native void nativeInit();
    private native void nativeDestroy(long nativeVpnNativeWorker);
    private native void nativeGetAllServerRegions(long nativeVpnNativeWorker);
    private native void nativeGetTimezonesForRegions(long nativeVpnNativeWorker);
    private native void nativeGetHostnamesForRegion(long nativeVpnNativeWorker, String region);
    private native void nativeGetSubscriberCredential(long nativeVpnNativeWorker,
            String productType, String productId, String validationMethod, String purchaseToken);
    private native void nativeVerifyPurchaseToken(
            long nativeVpnNativeWorker, String purchaseToken, String productId, String productType);
}
