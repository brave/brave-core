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
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;
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
                // I wish I would have OnceCallback in Java
                mUpdateCompleteCallback = null;
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

// TODO(AlexeyBarabash): give some best name
public void onInstallComplete(Runnable callback) {
    assert !mCachedWalletConfiguredOnAndroid;
    synchronized (mLock) {
        if (mUpdateCompleted) {
            callback.run();
        } else {
            mUpdateCompleteCallback = callback;
            // Sentinel task, we don't want to wait more than 10 sec
            PostTask.postDelayedTask(TaskTraits.UI_DEFAULT, () -> {
                Log.e(TAG, "DataFilesComponentInstaller.onInstallComplete.lambda 000");
                synchronized (mLock) {
                    if (mUpdateCompleteCallback != null) {
                        Log.e(TAG,
                                "DataFilesComponentInstaller.onInstallComplete.lambda 001 call mUpdateCompleteCallback.run()");
                        mUpdateCompleteCallback.run();
                        // I wish I would have OnceCallback in Java
                        mUpdateCompleteCallback = null;
                    }
                }
            }, 10 * 1000);
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
