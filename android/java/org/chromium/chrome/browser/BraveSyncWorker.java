/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import android.content.Context;
import android.content.SharedPreferences;

import org.chromium.base.Log;
import org.chromium.base.ContextUtils;
import org.chromium.base.ThreadUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;

import java.lang.Runnable;

@JNINamespace("chrome::android")
public class BraveSyncWorker {
    public static final String TAG = "SYNC";

    private Context mContext;
    private String mDebug = "true";

    private long mNativeBraveSyncWorker;

    @CalledByNative
    private void setNativePtr(long nativePtr) {
        assert mNativeBraveSyncWorker == 0;
        mNativeBraveSyncWorker = nativePtr;
    }

    private void Init() {
        if (mNativeBraveSyncWorker == 0) {
            nativeInit();
        }
    }

    @Override
    protected void finalize() {
        Destroy();
    }

    private void Destroy() {
        if (mNativeBraveSyncWorker != 0) {
            nativeDestroy(mNativeBraveSyncWorker);
            mNativeBraveSyncWorker = 0;
        }
    }

    public BraveSyncWorker() {
        mContext = ContextUtils.getApplicationContext();
        Init();
        (new MigrationFromV1()).MigrateFromSyncV1();
    }

    private class MigrationFromV1 {
        // Deprecated
        public static final String PREF_NAME = "SyncPreferences";
        private static final String PREF_LAST_FETCH_NAME = "TimeLastFetch";
        private static final String PREF_LATEST_DEVICE_RECORD_TIMESTAMPT_NAME =
                "LatestDeviceRecordTime";
        private static final String PREF_LAST_TIME_SEND_NOT_SYNCED_NAME = "TimeLastSendNotSynced";
        public static final String PREF_DEVICE_ID = "DeviceId";
        public static final String PREF_BASE_ORDER = "BaseOrder";
        public static final String PREF_LAST_ORDER = "LastOrder";
        public static final String PREF_SEED = "Seed";
        public static final String PREF_SYNC_DEVICE_NAME = "SyncDeviceName";
        private static final String PREF_SYNC_SWITCH = "sync_switch";
        private static final String PREF_SYNC_BOOKMARKS = "brave_sync_bookmarks";
        public static final String PREF_SYNC_TABS = "brave_sync_tabs"; // never used
        public static final String PREF_SYNC_HISTORY = "brave_sync_history"; // never used
        public static final String PREF_SYNC_AUTOFILL_PASSWORDS =
                "brave_sync_autofill_passwords"; // never used
        public static final String PREF_SYNC_PAYMENT_SETTINGS =
                "brave_sync_payment_settings"; // never used

        private boolean HaveSyncV1Prefs() {
            SharedPreferences sharedPref = mContext.getSharedPreferences(PREF_NAME, 0);

            String deviceId = sharedPref.getString(PREF_DEVICE_ID, null);
            if (null == deviceId) {
                return false;
            }
            return true;
        }

        private void DeleteSyncV1Prefs() {
            SharedPreferences sharedPref = mContext.getSharedPreferences(PREF_NAME, 0);
            SharedPreferences.Editor editor = sharedPref.edit();
            editor.clear().apply();
        }

        private void DeleteSyncV1LevelDb() {
            nativeDestroyV1LevelDb();
        }

        public void MigrateFromSyncV1() {
            // Do all migration work in file IO thread because we may need to
            // read shared preferences and delete level db
            PostTask.postTask(TaskTraits.BEST_EFFORT_MAY_BLOCK, () -> {
                if (HaveSyncV1Prefs()) {
                    Log.i(TAG, "Found sync v1 data, doing migration");
                    DeleteSyncV1Prefs();
                    DeleteSyncV1LevelDb();
                    // Mark sync v1 was enabled to trigger informers
                    ThreadUtils.runOnUiThreadBlocking(new Runnable() {
                        @Override
                        public void run() {
                            nativeMarkSyncV1WasEnabledAndMigrated();
                            BraveSyncInformers.show();
                        }
                    });
                }
            });
        }
    };

    public String GetCodephrase() {
        return nativeGetSyncCodeWords(mNativeBraveSyncWorker);
    }

    public void SaveCodephrase(String codephrase) {
        nativeSaveCodeWords(mNativeBraveSyncWorker, codephrase);
    }

    public String GetSeedHexFromWords(String codephrase) {
        return nativeGetSeedHexFromWords(codephrase);
    }

    public String GetWordsFromSeedHex(String seedHex) {
        return nativeGetWordsFromSeedHex(seedHex);
    }

    public void RequestSync() {
        nativeRequestSync(mNativeBraveSyncWorker);
    }

    public boolean IsFirstSetupComplete() {
        return nativeIsFirstSetupComplete(mNativeBraveSyncWorker);
    }

    public void FinalizeSyncSetup() {
        nativeFinalizeSyncSetup(mNativeBraveSyncWorker);
    }

    public boolean ResetSync() {
        return nativeResetSync(mNativeBraveSyncWorker);
    }

    public boolean getSyncV1WasEnabled() {
        return nativeGetSyncV1WasEnabled(mNativeBraveSyncWorker);
    }

    public boolean getSyncV2MigrateNoticeDismissed() {
        return nativeGetSyncV2MigrateNoticeDismissed(mNativeBraveSyncWorker);
    }

    public void setSyncV2MigrateNoticeDismissed(boolean isDismissed) {
        nativeSetSyncV2MigrateNoticeDismissed(mNativeBraveSyncWorker, isDismissed);
    }

    private native void nativeInit();
    private native void nativeDestroy(long nativeBraveSyncWorker);

    private native void nativeDestroyV1LevelDb();
    private native void nativeMarkSyncV1WasEnabledAndMigrated();

    private native String nativeGetSyncCodeWords(long nativeBraveSyncWorker);
    private native void nativeRequestSync(long nativeBraveSyncWorker);

    private native String nativeGetSeedHexFromWords(String passphrase);
    private native String nativeGetWordsFromSeedHex(String seedHex);
    private native void nativeSaveCodeWords(long nativeBraveSyncWorker, String passphrase);

    private native void nativeFinalizeSyncSetup(long nativeBraveSyncWorker);

    private native boolean nativeIsFirstSetupComplete(long nativeBraveSyncWorker);

    private native boolean nativeResetSync(long nativeBraveSyncWorker);

    private native boolean nativeGetSyncV1WasEnabled(long nativeBraveSyncWorker);
    private native boolean nativeGetSyncV2MigrateNoticeDismissed(long nativeBraveSyncWorker);
    private native void nativeSetSyncV2MigrateNoticeDismissed(long nativeBraveSyncWorker, boolean isDismissed);
}
