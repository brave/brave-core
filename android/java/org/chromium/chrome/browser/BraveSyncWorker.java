/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import android.content.Context;
import android.content.SharedPreferences;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;

import java.lang.Runnable;

@JNINamespace("chrome::android")
public class BraveSyncWorker {
    public static final String TAG = "SYNC";

    private Context mContext;
    private String mDebug = "true";

    private long mNativeBraveSyncWorker;

    private static BraveSyncWorker sBraveSyncWorker;
    private static boolean sInitialized;

    public static BraveSyncWorker get() {
        if (!sInitialized) {
            sBraveSyncWorker = new BraveSyncWorker();
            sInitialized = true;
        }
        return sBraveSyncWorker;
    }

    @CalledByNative
    private void setNativePtr(long nativePtr) {
        assert mNativeBraveSyncWorker == 0;
        mNativeBraveSyncWorker = nativePtr;
    }

    private void Init() {
        if (mNativeBraveSyncWorker == 0) {
            BraveSyncWorkerJni.get().init(BraveSyncWorker.this);
        }
    }

    @Override
    protected void finalize() {
        Destroy();
    }

    private void Destroy() {
        if (mNativeBraveSyncWorker != 0) {
            BraveSyncWorkerJni.get().destroy(mNativeBraveSyncWorker);
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
            BraveSyncWorkerJni.get().destroyV1LevelDb();
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
                            BraveSyncWorkerJni.get().markSyncV1WasEnabledAndMigrated();
                            BraveSyncInformers.show();
                        }
                    });
                }
            });
        }
    };

    public String GetCodephrase() {
        return BraveSyncWorkerJni.get().getSyncCodeWords(mNativeBraveSyncWorker);
    }

    public void SaveCodephrase(String codephrase) {
        BraveSyncWorkerJni.get().saveCodeWords(mNativeBraveSyncWorker, codephrase);
    }

    public String GetSeedHexFromWords(String codephrase) {
        return BraveSyncWorkerJni.get().getSeedHexFromWords(codephrase);
    }

    public String GetWordsFromSeedHex(String seedHex) {
        return BraveSyncWorkerJni.get().getWordsFromSeedHex(seedHex);
    }

    public void RequestSync() {
        BraveSyncWorkerJni.get().requestSync(mNativeBraveSyncWorker);
    }

    public boolean IsFirstSetupComplete() {
        return BraveSyncWorkerJni.get().isFirstSetupComplete(mNativeBraveSyncWorker);
    }

    public void FinalizeSyncSetup() {
        BraveSyncWorkerJni.get().finalizeSyncSetup(mNativeBraveSyncWorker);
    }

    public void ResetSync() {
        BraveSyncWorkerJni.get().resetSync(mNativeBraveSyncWorker);
    }

    public boolean getSyncV1WasEnabled() {
        return BraveSyncWorkerJni.get().getSyncV1WasEnabled(mNativeBraveSyncWorker);
    }

    public boolean getSyncV2MigrateNoticeDismissed() {
        return BraveSyncWorkerJni.get().getSyncV2MigrateNoticeDismissed(mNativeBraveSyncWorker);
    }

    public void setSyncV2MigrateNoticeDismissed(boolean isDismissed) {
        BraveSyncWorkerJni.get().setSyncV2MigrateNoticeDismissed(
                mNativeBraveSyncWorker, isDismissed);
    }

    @NativeMethods
    interface Natives {
        void init(BraveSyncWorker caller);
        void destroy(long nativeBraveSyncWorker);

        void destroyV1LevelDb();
        void markSyncV1WasEnabledAndMigrated();

        String getSyncCodeWords(long nativeBraveSyncWorker);
        void requestSync(long nativeBraveSyncWorker);

        String getSeedHexFromWords(String passphrase);
        String getWordsFromSeedHex(String seedHex);
        void saveCodeWords(long nativeBraveSyncWorker, String passphrase);

        void finalizeSyncSetup(long nativeBraveSyncWorker);

        boolean isFirstSetupComplete(long nativeBraveSyncWorker);

        void resetSync(long nativeBraveSyncWorker);

        boolean getSyncV1WasEnabled(long nativeBraveSyncWorker);
        boolean getSyncV2MigrateNoticeDismissed(long nativeBraveSyncWorker);
        void setSyncV2MigrateNoticeDismissed(long nativeBraveSyncWorker, boolean isDismissed);
    }
}
