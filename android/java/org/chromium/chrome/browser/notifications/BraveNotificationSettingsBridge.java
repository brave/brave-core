/**
 * Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.notifications;

import android.annotation.TargetApi;
import android.os.Build;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.chrome.browser.notifications.NotificationSettingsBridge;

/**
 * This class provides our native code to access NotificationSettingsBridge
 * without patching notification_channels_provider_android.cc/h.
 */
public class BraveNotificationSettingsBridge {
    @TargetApi(Build.VERSION_CODES.O)
    @CalledByNative
    static @NotificationChannelStatus int getChannelStatus(String channelId) {
        return NotificationSettingsBridge.getChannelStatus(channelId);
    }
}
