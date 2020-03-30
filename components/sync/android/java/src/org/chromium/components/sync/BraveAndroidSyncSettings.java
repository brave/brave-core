/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.components.sync;


public class BraveAndroidSyncSettings extends AndroidSyncSettings {
	public BraveAndroidSyncSettings(SyncContentResolverDelegate syncContentResolverDelegate) {
        super(syncContentResolverDelegate, null);
    }

    @Override
    protected void setChromeSyncEnabled(boolean value) {
    	mChromeSyncEnabled = false;
    	notifyObservers();
    }

    @Override
    protected boolean updateCachedSettings() {
    	boolean oldChromeSyncEnabled = mChromeSyncEnabled;
        boolean oldMasterSyncEnabled = mMasterSyncEnabled;

        mIsSyncable = false;
        mChromeSyncEnabled = false;
        mMasterSyncEnabled = false;

        return oldChromeSyncEnabled != mChromeSyncEnabled
                || oldMasterSyncEnabled != mMasterSyncEnabled;
    }

    public void notifyObservers() {
        assert false;
    }
}
