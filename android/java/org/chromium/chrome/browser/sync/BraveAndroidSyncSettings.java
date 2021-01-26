/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.sync;

import android.accounts.Account;

import org.chromium.chrome.browser.sync.ProfileSyncService;

// see org.brave.bytecode.BraveAndroidSyncSettingsAdapter
public class BraveAndroidSyncSettings extends AndroidSyncSettings {
    private boolean mMasterSyncEnabled;

    public BraveAndroidSyncSettings(Account account) {
        super(account);
    }

    @Override
    public void disableChromeSync() { }

    // We need to override this to make able
    // DevicePickerBottomSheetContent.createContentView send the link
    // For Brave we don't have an account in Android system account,
    // so pretend sync for Brave "account" is always on when sync is configured
    @Override
    public boolean isChromeSyncEnabled() {
        ProfileSyncService profileSyncService = ProfileSyncService.get();
        return profileSyncService != null && profileSyncService.isFirstSetupComplete();
    }
}
