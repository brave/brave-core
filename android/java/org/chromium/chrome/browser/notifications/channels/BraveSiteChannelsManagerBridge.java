/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.notifications.channels;

import android.app.NotificationChannel;

import org.jni_zero.CalledByNative;

import org.chromium.base.BraveReflectionUtil;
import org.chromium.chrome.browser.notifications.NotificationChannelStatus;
import org.chromium.components.browser_ui.notifications.NotificationManagerProxyImpl;

/**
 * This class provides our native code to access NotificationManagerProxy/SiteChannelsManager
 * without patching notification_channels_provider_android.cc/h.
 */
public class BraveSiteChannelsManagerBridge {
    @CalledByNative
    static @NotificationChannelStatus int getChannelStatus(String channelId) {
        // NotificationManagerProxy.getNotificationChannel is now deprecated
        // But it goes eventually to Android API
        // androidx.core.app.NotificationManagerCompat.getNotificationChannel,
        // see components/browser_ui/notifications/NotificationManagerProxyImpl.java
        NotificationChannel channel =
                NotificationManagerProxyImpl.getInstance().getNotificationChannel(channelId);
        if (channel == null) return NotificationChannelStatus.UNAVAILABLE;

        return (int)
                BraveReflectionUtil.invokeMethod(
                        SiteChannelsManager.class,
                        null,
                        "toChannelStatus",
                        int.class,
                        channel.getImportance());
    }
}
