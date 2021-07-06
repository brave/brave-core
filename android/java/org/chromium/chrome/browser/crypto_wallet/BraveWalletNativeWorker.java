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
                instance.init();
            }
        }
        return instance;
    }

    private BraveWalletNativeWorker() {}

    private void init() {
        if (mNativeBraveWalletNativeWorker == 0) {
            BraveWalletNativeWorkerJni.get().init(this);
        }
    }

    @Override
    protected void finalize() {
        destroy();
    }

    private void destroy() {
        if (mNativeBraveWalletNativeWorker != 0) {
            BraveWalletNativeWorkerJni.get().destroy(mNativeBraveWalletNativeWorker, this);
            mNativeBraveWalletNativeWorker = 0;
        }
    }

    @CalledByNative
    private void setNativePtr(long nativePtr) {
        assert mNativeBraveWalletNativeWorker == 0;
        mNativeBraveWalletNativeWorker = nativePtr;
    }

    public String getRecoveryWords() {
        return BraveWalletNativeWorkerJni.get().getRecoveryWords(mNativeBraveWalletNativeWorker);
    }

    public boolean isWalletLocked() {
        return BraveWalletNativeWorkerJni.get().isWalletLocked(mNativeBraveWalletNativeWorker);
    }

    public String createWallet(String password) {
        return BraveWalletNativeWorkerJni.get().createWallet(
                mNativeBraveWalletNativeWorker, password);
    }

    public void lockWallet() {
        BraveWalletNativeWorkerJni.get().lockWallet(mNativeBraveWalletNativeWorker);
    }

    public boolean unlockWallet(String password) {
        return BraveWalletNativeWorkerJni.get().unlockWallet(
                mNativeBraveWalletNativeWorker, password);
    }

    public String restoreWallet(String mnemonic, String password) {
        return BraveWalletNativeWorkerJni.get().restoreWallet(
                mNativeBraveWalletNativeWorker, mnemonic, password);
    }

    public void resetWallet() {
        BraveWalletNativeWorkerJni.get().resetWallet(mNativeBraveWalletNativeWorker);
    }

    @NativeMethods
    interface Natives {
        void init(BraveWalletNativeWorker caller);
        void destroy(long nativeBraveWalletNativeWorker, BraveWalletNativeWorker caller);
        String getRecoveryWords(long nativeBraveWalletNativeWorker);
        boolean isWalletLocked(long nativeBraveWalletNativeWorker);
        String createWallet(long nativeBraveWalletNativeWorker, String password);
        void lockWallet(long nativeBraveWalletNativeWorker);
        boolean unlockWallet(long nativeBraveWalletNativeWorker, String password);
        String restoreWallet(long nativeBraveWalletNativeWorker, String mnemonic, String password);
        void resetWallet(long nativeBraveWalletNativeWorker);
    }
}
