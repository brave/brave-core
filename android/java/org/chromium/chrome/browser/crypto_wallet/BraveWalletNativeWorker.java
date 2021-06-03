/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;

@JNINamespace("chrome::android")
public class BraveWalletNativeWorker {
    private long mNativeBraveWalletNativeWorker;
    private static final Object lock = new Object();
    private static BraveWalletNativeWorker instance;

    public static BraveWalletNativeWorker getInstance() {
        synchronized (lock) {
            if (instance == null) {
                instance = new BraveWalletNativeWorker();
                instance.Init();
            }
        }
        return instance;
    }

    private BraveWalletNativeWorker() {
    }

    private void Init() {
        if (mNativeBraveWalletNativeWorker == 0) {
            BraveWalletNativeWorkerJni.get().init(this);
        }
    }

    @Override
    protected void finalize() {
        Destroy();
    }

    private void Destroy() {
        if (mNativeBraveWalletNativeWorker != 0) {
            BraveWalletNativeWorkerJni.get().destroy(
                mNativeBraveWalletNativeWorker, this);
            mNativeBraveWalletNativeWorker = 0;
        }
    }

    @CalledByNative
    private void setNativePtr(long nativePtr) {
        assert mNativeBraveWalletNativeWorker == 0;
        mNativeBraveWalletNativeWorker = nativePtr;
    }

    public String CreateWallet(String password) {
        return BraveWalletNativeWorkerJni.get().createWallet(
            mNativeBraveWalletNativeWorker, password);
    }

    public void LockWallet() {
        BraveWalletNativeWorkerJni.get().lockWallet(
            mNativeBraveWalletNativeWorker);
    }

    public void UnlockWallet(String password) {
        BraveWalletNativeWorkerJni.get().unlockWallet(
            mNativeBraveWalletNativeWorker, password);
    }

    @NativeMethods
    interface Natives {
        void init(BraveWalletNativeWorker caller);
        void destroy(long nativeBraveWalletNativeWorker,
            BraveWalletNativeWorker caller);
        String createWallet(long nativeBraveWalletNativeWorker, String password);
        void lockWallet(long nativeBraveWalletNativeWorker);
        boolean unlockWallet(long nativeBraveWalletNativeWorker, String password);
    }
}
