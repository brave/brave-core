/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.sync;

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
}
