/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.sync;

import java.util.ArrayList;

import org.chromium.chrome.browser.sync.BraveSyncServiceObserver;

public class BraveSyncService {
    public static final int NICEWARE_WORD_COUNT = 16;
    public static final int BIP39_WORD_COUNT = 24;

    private static BraveSyncService sInstance;
    private static final Object mLock = new Object();

    public static BraveSyncService getInstance() {
        synchronized (mLock) {
            if (sInstance == null) {
                sInstance = new BraveSyncService();
            }
        }
        return sInstance;
    }

    private BraveSyncService() {}

    class BookmarkInternal {
        public BookmarkInternal() {}
    }

    public class ResolvedRecordToApply implements Comparable<ResolvedRecordToApply> {
        public ResolvedRecordToApply(String objectId, String action,
                BookmarkInternal bookMarkInternal, String deviceName, String deviceId,
                long syncTime) {}

        public String mObjectId;
        public String mDeviceName;
        public String mDeviceId;

        @Override
        public int compareTo(ResolvedRecordToApply compare) {
            return 0;
        }
    }

    public void onSetupSyncHaveCode(String syncWords, String deviceName) {}
    public void onSetupSyncNewToSync(String deviceName) {}
    public void onDeleteDevice(String deviceId) {}
    public void onResetSync() {}

    public interface GetSettingsAndDevicesCallback {
        public void onGetSettingsAndDevices(ArrayList<ResolvedRecordToApply> devices);
    }

    public void getSettingsAndDevices(GetSettingsAndDevicesCallback callback) {}

    public void getSyncWords() {}
    public String getSeed() {
        return "";
    }

    public void onSetSyncEnabled(boolean enabled) {}
    public void onSetSyncBookmarks(boolean syncBookmarks) {}
    public void onSetSyncBrowsingHistory(boolean syncBrowsingHistory) {}
    public void onSetSyncSavedSiteSettings(boolean syncSavedSiteSettings) {}
}
