/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import android.content.Context;
import android.content.SharedPreferences;

import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;

import java.lang.Runnable;

@JNINamespace("chrome::android")
public class BraveSyncWorker {
    public static final String TAG = "SYNC";
    public static final String PREF_NAME = "SyncPreferences";
    private static final String PREF_LAST_FETCH_NAME = "TimeLastFetch";
    private static final String PREF_LATEST_DEVICE_RECORD_TIMESTAMPT_NAME = "LatestDeviceRecordTime";
    private static final String PREF_LAST_TIME_SEND_NOT_SYNCED_NAME = "TimeLastSendNotSynced";
    public static final String PREF_DEVICE_ID = "DeviceId";
    public static final String PREF_BASE_ORDER = "BaseOrder";
    public static final String PREF_LAST_ORDER = "LastOrder";
    public static final String PREF_SEED = "Seed";
    public static final String PREF_SYNC_DEVICE_NAME = "SyncDeviceName";
    private static final String PREF_SYNC_SWITCH = "sync_switch";
    private static final String PREF_SYNC_BOOKMARKS = "brave_sync_bookmarks";
    public static final String PREF_SYNC_TABS = "brave_sync_tabs";
    public static final String PREF_SYNC_HISTORY = "brave_sync_history";
    public static final String PREF_SYNC_AUTOFILL_PASSWORDS = "brave_sync_autofill_passwords";
    public static final String PREF_SYNC_PAYMENT_SETTINGS = "brave_sync_payment_settings";

    private Context mContext;
    private String mDebug = "true";

    public BraveSyncWorker(Context context) {
        mContext = context;
    }
  }
