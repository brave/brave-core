/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.components.sync;

import java.lang.reflect.Method;

//import org.chromium.chrome.browser.sync.ProfileSyncService;
//import org.chromium.components.sync.AndroidSyncSettings;
import org.chromium.components.sync.SyncContentResolverDelegate;
  import org.chromium.base.Log;

// see org.brave.bytecode.BraveAndroidSyncSettingsAdapter
public class BraveAndroidSyncSettings extends AndroidSyncSettings {
    private boolean mMasterSyncEnabled;

    public BraveAndroidSyncSettings(SyncContentResolverDelegate syncContentResolverDelegate) {
        super(syncContentResolverDelegate, null);
    }

    // Chromium's AndroidSyncSettings.mChromeSyncEnabled is never set to true
    // after app restart when Brave sync v2 is enabled, and by this reason on
    // next SyncController.updateSyncStateFromAndroid sync gets disabled, so
    // we act here as mChromeSyncEnabled would be true.
    // Following up issue https://github.com/brave/brave-browser/issues/10454
    @Override
    public boolean isSyncEnabled() {
        return mMasterSyncEnabled;
    }

    @Override
    public void disableChromeSync() { }

    // We need to override this to make able
    // DevicePickerBottomSheetContent.createContentView send the link
    // For Brave we don't have an account in Android system account,
    // so pretend sync for Brave "account" is always on when sync is requsted
    @Override
    public boolean isChromeSyncEnabled() {
        // On Chrome 84 ProfileSyncService.java is in //chrome/android:chrome_java
        // but AndroidSyncSettings.java is in //components/sync/android:sync_java
        // //chrome/android:chrome_java depends on //components/sync/android:sync_java
        // so I cannot add deps to //components/sync/android:sync_java
        // Therefore doing with reflection
        try {
            Method methodGet = Class.forName("org.chromium.chrome.browser.sync.ProfileSyncService").getDeclaredMethod("get");
            Object profileSyncService = methodGet.invoke(null);
            if (profileSyncService == null) {
                return false;
            }

            Method methodIsSyncRequested = Class.forName("org.chromium.chrome.browser.sync.ProfileSyncService").getDeclaredMethod("isSyncRequested");
            Object isSyncRequested = methodIsSyncRequested.invoke(profileSyncService);
            boolean bIsSyncRequested = (boolean)isSyncRequested;
            return bIsSyncRequested;
        } catch (Exception e) {
            assert false;
            // failsafe, but should never reach
            return true;
        }
    }
}
