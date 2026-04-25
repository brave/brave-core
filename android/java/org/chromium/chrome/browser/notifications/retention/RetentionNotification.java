/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.notifications.retention;

public class RetentionNotification {

    private final int mNotificationId;
    private final int mNotificationTime; // In minutes
    private final String mChannelId;
    private final String mNotificationTitle;

    public RetentionNotification(
            int notificationId, int notificationTime, String channelId, String notificationTitle) {
        mNotificationId = notificationId;
        mNotificationTime = notificationTime;
        mChannelId = channelId;
        mNotificationTitle = notificationTitle;
    }

    public int getNotificationId() {
        return mNotificationId;
    }

    public int getNotificationTime() {
        return mNotificationTime;
    }

    public String getChannelId() {
        return mChannelId;
    }

    public String getNotificationTitle() {
        return mNotificationTitle;
    }
}
