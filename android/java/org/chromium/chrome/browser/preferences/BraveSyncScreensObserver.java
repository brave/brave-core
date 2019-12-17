/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.preferences;

/**
 * Allows monitoring of JavaScript results.
 */
public interface BraveSyncScreensObserver {
    /**
     * Informs when the words code provided is incorrect
     */
    public void onSyncError(String message);

    /**
     * Informs when the seed in received
     */
    public void onSeedReceived(String seed, boolean fromCodeWords, boolean afterInitialization);

    /**
     * Informs when the code words are received
     */
    public void onCodeWordsReceived(String[] codeWords);

    /**
     * Informs when the list of devices is available
     */
    public void onDevicesAvailable();

    /**
     * Informs when the sync is reset
     */
    public void onResetSync();

    /**
     * Returns true if sync settings with devices list is currently shown
     */
    public boolean shouldLoadDevices();
}
