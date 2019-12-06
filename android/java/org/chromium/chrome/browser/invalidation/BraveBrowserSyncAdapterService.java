/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.invalidation;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;

/**
 * A Service that provides access to {@link BraveBrowserSyncAdapter}.
 */
public class BraveBrowserSyncAdapterService extends Service {
    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }
}
