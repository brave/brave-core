/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import org.chromium.base.ContextUtils;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;
import org.chromium.chrome.browser.InternetConnection;
import org.chromium.chrome.browser.component_updater.BraveComponentUpdater;

/**
 * Class that manages Brave Wallet Data files component installation for Android
 */
public class DataFilesComponentInstaller implements BraveComponentUpdater.ComponentUpdaterListener {
    private static final String WALLET_COMPONENT_ID =
            WalletDataFilesInstaller.getWalletDataFilesComponentId();
    private Object mLock = new Object();
    private boolean mUpdateStarted;
    private boolean mUpdateCompleted;
    Runnable mUpdateCompleteCallback;

    /**
     * Callback to be notified about change of component downloading
     */
    public interface InfoCallback {
        void onInfo(String info);
        void onProgress(long downloadedBytes, long totalBytes);
        void onDownloadUpdateComplete();
    }
    private InfoCallback mInfoCallback;

    private boolean mCachedWalletConfiguredOnAndroid; // TODO(AlexeyBarabash): better name?

    public void setInfoCallback(InfoCallback infoCallback) {
        mInfoCallback = infoCallback;
    }

    public void setCachedWalletConfiguredOnAndroid(boolean value) {
        mCachedWalletConfiguredOnAndroid = value;
    }

    public void registerAndInstallEx() {
        assert !WalletDataFilesInstallerUtil.isBraveWalletConfiguredOnAndroid();

        synchronized (mLock) {
            assert !mUpdateStarted;
            assert !mUpdateCompleted;

            mUpdateStarted = true;
            WalletDataFilesInstaller.registerWalletDataFilesComponentOnDemand(new Runnable() {
                @Override
                public void run() {
                    synchronized (mLock) {
                        mUpdateStarted = false;
                        mUpdateCompleted = true;
                        if (mUpdateCompleteCallback != null) {
                            mUpdateCompleteCallback.run();
                            mUpdateCompleteCallback = null;

                            if (mInfoCallback != null) {
                                mInfoCallback.onDownloadUpdateComplete();
                                mInfoCallback = null;
                            }
                            BraveComponentUpdater.get().removeComponentUpdateEventListener(
                                    DataFilesComponentInstaller.this);
                        }
                    }
                }
            });

            BraveComponentUpdater.get().addComponentUpdateEventListener(this);
        }
    }

    public void registerListener() {
        synchronized (mLock) {
            BraveComponentUpdater.get().addComponentUpdateEventListener(this);
        }
    }

    @Override
    public void onComponentUpdateEvent(int event, String id) {
        if (WALLET_COMPONENT_ID.equals(id)) {
            BraveComponentUpdater.CrxUpdateItem item =
                    BraveComponentUpdater.get().getUpdateState(id);
            if (item.mDownloadedBytes > 0 && item.mTotalBytes > 0 && mInfoCallback != null) {
                mInfoCallback.onProgress(item.mDownloadedBytes, item.mTotalBytes);
            }
        }
    }

    public boolean needToWaitComponentLoad() {
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

    // TODO(AlexeyBarabash): give some better name
    public void onInstallComplete(Runnable callback) {
        assert !mCachedWalletConfiguredOnAndroid;
        synchronized (mLock) {
            if (mUpdateCompleted) {
                callback.run();
            } else {
                mUpdateCompleteCallback = callback;
                // Sentinel task, we don't want to wait more than 10 sec
                // TODO(AlexeyBarabash), how to cancel?
                // FirstRunAppRestrictionInfo.java - but task is not delayed
                PostTask.postDelayedTask(TaskTraits.UI_DEFAULT,
                        new Runnable() {
                            @Override
                            public void run() {
                                synchronized (mLock) {
                                    if (mUpdateCompleteCallback != null) {
                                        mUpdateCompleteCallback.run();
                                        mUpdateCompleteCallback = null;

                                        if (mInfoCallback != null) {
                                            mInfoCallback.onDownloadUpdateComplete();
                                            mInfoCallback = null;
                                        }
                                        BraveComponentUpdater.get()
                                                .removeComponentUpdateEventListener(
                                                        DataFilesComponentInstaller.this);
                                    }
                                }
                            }
                        },
                        // TODO(AlexeyBarabash): move to constant or get rid of this
                        5 * 60 * 1000);
            }
        }
    }
}
