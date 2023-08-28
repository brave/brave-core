/**
 * Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.playlist.download;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

import org.chromium.chrome.browser.playlist.download.DownloadService;

public class CancelDownloadBroadcastReceiver extends BroadcastReceiver {
    public static String CANCEL_DOWNLOAD_ACTION = "cancel_download_action";

    @Override
    public void onReceive(Context context, Intent intent) {
        String action = intent.getAction();
        if (action != null && action.equals(CANCEL_DOWNLOAD_ACTION)) {
            context.stopService(new Intent(context, DownloadService.class));
        }
    }
}
