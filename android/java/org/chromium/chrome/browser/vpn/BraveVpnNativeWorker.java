/**
 * Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.vpn;

import org.jni_zero.CalledByNative;
import org.jni_zero.JNINamespace;
import org.jni_zero.NativeMethods;

import java.util.ArrayList;
import java.util.List;

@JNINamespace("chrome::android")
public class BraveVpnNativeWorker {
    private long mNativeBraveVpnNativeWorker;
    private static final Object sLock = new Object();
    private static BraveVpnNativeWorker sInstance;

    private List<BraveVpnObserver> mObservers;

    public static BraveVpnNativeWorker getInstance() {
        synchronized (sLock) {
            if (sInstance == null) {
                sInstance = new BraveVpnNativeWorker();
                sInstance.init();
            }
        }
        return sInstance;
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
        synchronized (sLock) {
            mObservers.add(observer);
        }
    }

    public void removeObserver(BraveVpnObserver observer) {
        synchronized (sLock) {
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
    public void onGetServerRegionsWithCities(String jsonServerRegions, boolean isSuccess) {
        for (BraveVpnObserver observer : mObservers) {
            observer.onGetServerRegionsWithCities(jsonServerRegions, isSuccess);
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
    public void onGetWireguardProfileCredentials(
            String jsonWireguardProfileCredentials, boolean isSuccess) {
        for (BraveVpnObserver observer : mObservers) {
            observer.onGetWireguardProfileCredentials(jsonWireguardProfileCredentials, isSuccess);
        }
    }

    @CalledByNative
    public void onVerifyCredentials(String jsonVerifyCredentials, boolean isSuccess) {
        for (BraveVpnObserver observer : mObservers) {
            observer.onVerifyCredentials(jsonVerifyCredentials, isSuccess);
        }
    }

    @CalledByNative
    public void onInvalidateCredentials(String jsonInvalidateCredentials, boolean isSuccess) {
        for (BraveVpnObserver observer : mObservers) {
            observer.onInvalidateCredentials(jsonInvalidateCredentials, isSuccess);
        }
    }

    @CalledByNative
    public void onGetSubscriberCredential(String subscriberCredential, boolean isSuccess) {
        for (BraveVpnObserver observer : mObservers) {
            observer.onGetSubscriberCredential(subscriberCredential, isSuccess);
        }
    }

    @CalledByNative
    public void onVerifyPurchaseToken(
            String jsonResponse, String purchaseToken, String productId, boolean isSuccess) {
        for (BraveVpnObserver observer : mObservers) {
            observer.onVerifyPurchaseToken(jsonResponse, purchaseToken, productId, isSuccess);
        }
    }

    public void getAllServerRegions() {
        BraveVpnNativeWorkerJni.get().getAllServerRegions(mNativeBraveVpnNativeWorker);
    }

    public void getServerRegionsWithCities() {
        BraveVpnNativeWorkerJni.get().getServerRegionsWithCities(mNativeBraveVpnNativeWorker);
    }

    public void getTimezonesForRegions() {
        BraveVpnNativeWorkerJni.get().getTimezonesForRegions(mNativeBraveVpnNativeWorker);
    }

    public void getHostnamesForRegion(String region) {
        BraveVpnNativeWorkerJni.get().getHostnamesForRegion(mNativeBraveVpnNativeWorker, region);
    }

    public void getWireguardProfileCredentials(
            String subscriberCredential, String publicKey, String hostname) {
        BraveVpnNativeWorkerJni.get().getWireguardProfileCredentials(
                mNativeBraveVpnNativeWorker, subscriberCredential, publicKey, hostname);
    }

    public void verifyCredentials(
            String hostname, String clientId, String subscriberCredential, String apiAuthToken) {
        BraveVpnNativeWorkerJni.get().verifyCredentials(mNativeBraveVpnNativeWorker, hostname,
                clientId, subscriberCredential, apiAuthToken);
    }

    public void invalidateCredentials(
            String hostname, String clientId, String subscriberCredential, String apiAuthToken) {
        BraveVpnNativeWorkerJni.get().invalidateCredentials(mNativeBraveVpnNativeWorker, hostname,
                clientId, subscriberCredential, apiAuthToken);
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

    // Desktop purchase methods
    public void reloadPurchasedState() {
        BraveVpnNativeWorkerJni.get().reloadPurchasedState(mNativeBraveVpnNativeWorker);
    }

    public boolean isPurchasedUser() {
        return BraveVpnNativeWorkerJni.get().isPurchasedUser(mNativeBraveVpnNativeWorker);
    }

    public void getSubscriberCredentialV12() {
        BraveVpnNativeWorkerJni.get().getSubscriberCredentialV12(mNativeBraveVpnNativeWorker);
    }

    public void reportBackgroundP3A(long sessionStartTimeMs, long sessionEndTimeMs) {
        BraveVpnNativeWorkerJni.get().reportBackgroundP3A(
                mNativeBraveVpnNativeWorker, sessionStartTimeMs, sessionEndTimeMs);
    }

    public void reportForegroundP3A() {
        BraveVpnNativeWorkerJni.get().reportForegroundP3A(mNativeBraveVpnNativeWorker);
    }

    @NativeMethods
    interface Natives {
        void init(BraveVpnNativeWorker caller);

        void destroy(long nativeBraveVpnNativeWorker, BraveVpnNativeWorker caller);

        void getAllServerRegions(long nativeBraveVpnNativeWorker);

        void getServerRegionsWithCities(long nativeBraveVpnNativeWorker);

        void getTimezonesForRegions(long nativeBraveVpnNativeWorker);

        void getHostnamesForRegion(long nativeBraveVpnNativeWorker, String region);

        void getWireguardProfileCredentials(
                long nativeBraveVpnNativeWorker,
                String subscriberCredential,
                String publicKey,
                String hostname);

        void verifyCredentials(
                long nativeBraveVpnNativeWorker,
                String hostname,
                String clientId,
                String subscriberCredential,
                String apiAuthToken);

        void invalidateCredentials(
                long nativeBraveVpnNativeWorker,
                String hostname,
                String clientId,
                String subscriberCredential,
                String apiAuthToken);

        void getSubscriberCredential(
                long nativeBraveVpnNativeWorker,
                String productType,
                String productId,
                String validationMethod,
                String purchaseToken,
                String packageName);

        void verifyPurchaseToken(
                long nativeBraveVpnNativeWorker,
                String purchaseToken,
                String productId,
                String productType,
                String packageName);

        void reloadPurchasedState(long nativeBraveVpnNativeWorker);

        boolean isPurchasedUser(long nativeBraveVpnNativeWorker);
        void getSubscriberCredentialV12(long nativeBraveVpnNativeWorker);
        void reportBackgroundP3A(
                long nativeBraveVpnNativeWorker, long sessionStartTimeMs, long sessionEndTimeMs);
        void reportForegroundP3A(long nativeBraveVpnNativeWorker);
    }
}
