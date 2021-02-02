/**
 * Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.vpn;

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
                instance.Init();
            }
        }
        return instance;
    }

    private VpnNativeWorker() {
        mObservers = new ArrayList<VpnObserver>();
    }

    private void Init() {
        if (mNativeVpnNativeWorker == 0) {
            nativeInit();
        }
    }

    @Override
    protected void finalize() {
        Destroy();
    }

    private void Destroy() {
        if (mNativeVpnNativeWorker != 0) {
            nativeDestroy(mNativeVpnNativeWorker);
            mNativeVpnNativeWorker = 0;
        }
    }

    public void AddObserver(VpnObserver observer) {
        synchronized (lock) {
            mObservers.add(observer);
        }
    }

    public void RemoveObserver(VpnObserver observer) {
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
    public void OnGetAllServerRegions(String jsonBalances, boolean isSuccess) {
        for (VpnObserver observer : mObservers) {
            observer.OnGetAllServerRegions(jsonBalances, isSuccess);
        }
    }

    public void getAllServerRegions() {
        nativeGetAllServerRegions(mNativeVpnNativeWorker);
    }

    private native void nativeInit();
    private native void nativeDestroy(long nativeVpnNativeWorker);
    private native void nativeGetAllServerRegions(long nativeVpnNativeWorker);
}
