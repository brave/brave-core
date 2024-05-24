/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.notifications.channels;

import org.jni_zero.CalledByNative;

import org.chromium.chrome.browser.notifications.NotificationChannelStatus;

/**
 * This class provides our native code to access SiteChannelsManager without patching
 * notification_channels_provider_android.cc/h.
 */
public class BraveSiteChannelsManagerBridge {
    @CalledByNative
    static @NotificationChannelStatus int getChannelStatus(String channelId) {
        return SiteChannelsManager.getInstance().getChannelStatus(channelId);
    }
}
