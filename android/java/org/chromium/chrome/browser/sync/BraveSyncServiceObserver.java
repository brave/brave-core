/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.sync;

/**
 * Allows observing sync service events.
 */
public interface BraveSyncServiceObserver {
    /**
     * Informs when sync setup has an error.
     */
    public void onSyncSetupError(String message);

    /**
     * Informs when the sync state has changed.
     */
    public void onSyncStateChanged();

    /**
     * Informs when the sync words are received.
     */
    public void onHaveSyncWords(String[] syncWords);
}
