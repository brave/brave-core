/**
 * Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.vpn;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.chrome.browser.vpn.BraveVpnObserver;

import java.util.ArrayList;
import java.util.List;

@JNINamespace("chrome::android")
public class BraveVpnNativeWorker {
    private long mNativeBraveVpnNativeWorker;
    private static final Object mLock = new Object();
    private static BraveVpnNativeWorker mInstance;

    private List<BraveVpnObserver> mObservers;

    public static BraveVpnNativeWorker getInstance() {
        synchronized (mLock) {
            if (mInstance == null) {
                mInstance = new BraveVpnNativeWorker();
                mInstance.init();
            }
        }
        return mInstance;
    }

    private BraveVpnNativeWorker() {
        mObservers = new ArrayList<BraveVpnObserver>();
    }

    private void init() {
        if (mNativeBraveVpnNativeWorker == 0) {
            BraveVpnNativeWorkerJni.get().init(this);
        }
    }

    @Override
    protected void finalize() {
        destroy();
    }

    private void destroy() {
        if (mNativeBraveVpnNativeWorker != 0) {
            BraveVpnNativeWorkerJni.get().destroy(mNativeBraveVpnNativeWorker, this);
            mNativeBraveVpnNativeWorker = 0;
        }
    }

    public void addObserver(BraveVpnObserver observer) {
        synchronized (mLock) {
            mObservers.add(observer);
        }
    }

    public void removeObserver(BraveVpnObserver observer) {
        synchronized (mLock) {
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
    public void onGetProfileCredentials(String jsonProfileCredentials, boolean isSuccess) {
        for (BraveVpnObserver observer : mObservers) {
            observer.onGetProfileCredentials(jsonProfileCredentials, isSuccess);
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
        BraveVpnNativeWorkerJni.get().getAllServerRegions(mNativeBraveVpnNativeWorker);
    }

    public void getTimezonesForRegions() {
        BraveVpnNativeWorkerJni.get().getTimezonesForRegions(mNativeBraveVpnNativeWorker);
    }

    public void getHostnamesForRegion(String region) {
        BraveVpnNativeWorkerJni.get().getHostnamesForRegion(mNativeBraveVpnNativeWorker, region);
    }

    public void getProfileCredentials(String subscriberCredential, String hostname) {
        BraveVpnNativeWorkerJni.get().getProfileCredentials(
                mNativeBraveVpnNativeWorker, subscriberCredential, hostname);
    }

    public void getSubscriberCredential(String productType, String productId,
            String validationMethod, String purchaseToken, String packageName) {
        BraveVpnNativeWorkerJni.get().getSubscriberCredential(mNativeBraveVpnNativeWorker,
                productType, productId, validationMethod, purchaseToken, packageName);
    }

    public void verifyPurchaseToken(
            String purchaseToken, String productId, String productType, String packageName) {
        BraveVpnNativeWorkerJni.get().verifyPurchaseToken(
                mNativeBraveVpnNativeWorker, purchaseToken, productId, productType, packageName);
    }

    @NativeMethods
    interface Natives {
        void init(BraveVpnNativeWorker caller);
        void destroy(long nativeBraveVpnNativeWorker, BraveVpnNativeWorker caller);
        void getAllServerRegions(long nativeBraveVpnNativeWorker);
        void getTimezonesForRegions(long nativeBraveVpnNativeWorker);
        void getHostnamesForRegion(long nativeBraveVpnNativeWorker, String region);
        void getProfileCredentials(
                long nativeBraveVpnNativeWorker, String subscriberCredential, String hostname);
        void getSubscriberCredential(long nativeBraveVpnNativeWorker, String productType,
                String productId, String validationMethod, String purchaseToken,
                String packageName);
        void verifyPurchaseToken(long nativeBraveVpnNativeWorker, String purchaseToken,
                String productId, String productType, String packageName);
    }
}
