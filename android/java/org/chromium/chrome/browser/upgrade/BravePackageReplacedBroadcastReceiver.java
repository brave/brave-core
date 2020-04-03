/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.upgrade;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

import org.chromium.chrome.browser.preferences.BravePreferenceKeys;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;

/**
 * Triggered when Brave's package is replaced (e.g. when it is
 * upgraded).
 *
 * See important lifecycle notes in PackageReplacedBroadcastReceiver.
 */
public final class BravePackageReplacedBroadcastReceiver extends BroadcastReceiver {
    @Override
    public void onReceive(final Context context, Intent intent) {
        if (!Intent.ACTION_MY_PACKAGE_REPLACED.equals(intent.getAction())) return;
        BraveUpgradeJobIntentService.startMigrationIfNecessary(context);
        BravePrefServiceBridge.setInteger(BravePreferenceKeys.PREF_BRAVE_APP_OPEN_COUNT, 0);
    }
}
