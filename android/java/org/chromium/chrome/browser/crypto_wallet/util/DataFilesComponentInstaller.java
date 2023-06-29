/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import org.chromium.base.Callback;
import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.chrome.browser.InternetConnection;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;

/**
 * class DataFilesComponentInstaller
 */
public class DataFilesComponentInstaller {
    private static final String TAG = "DataFilesComponentInstaller";
    private Object mLock = new Object();
    private boolean mUpdateStarted;
    private boolean mUpdateCompleted;
    Runnable mUpdateCompleteCallback;

    private boolean mCachedWalletConfiguredOnAndroid;

    public void setCachedWalletConfiguredOnAndroid(boolean value) {
        mCachedWalletConfiguredOnAndroid = value;
    }

    public void registerAndInstallEx() {
        assert !isBraveWalletConfiguredOnAndroid();

        synchronized (mLock) {
            assert !mUpdateStarted;
            assert !mUpdateCompleted;

            mUpdateStarted = true;
            registerAndInstall((Boolean result) -> {
                synchronized (mLock) {
                    mUpdateStarted = false;
                    mUpdateCompleted = true;
                    if (mUpdateCompleteCallback != null) {
                mUpdateCompleteCallback.run();
                    }
        }
    });
}
}

public boolean needToWaitComponentLoad() {
    Log.e(TAG, "DataFilesComponentInstaller.needToWaitComponentLoad 000");
    if (mCachedWalletConfiguredOnAndroid) {
        return false;
    }

    if (!InternetConnection.isNetworkAvailable(ContextUtils.getApplicationContext())) {
        return false;
    }

    synchronized (mLock) {
        return mUpdateStarted;
    }
}

public void onInstallComplete(Runnable callback) {
    assert !mCachedWalletConfiguredOnAndroid;
    synchronized (mLock) {
        if (mUpdateCompleted) {
            callback.run();
        } else {
            mUpdateCompleteCallback = callback;
        }
    }
}

@CalledByNative
private static boolean isBraveWalletConfiguredOnAndroid() {
    return !Utils.shouldShowCryptoOnboarding();
}

public static void registerAndInstall(Callback<Boolean> callback) {
    DataFilesComponentInstallerJni.get().registerAndInstall(callback);
}

@CalledByNative
private static void onRegisterAndInstallDone(Callback<Boolean> callback, Boolean result) {
    callback.onResult(result);
}

@NativeMethods
interface Natives {
    void registerAndInstall(Callback<Boolean> callback);
}
}
