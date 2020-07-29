/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.notifications.retention;

public class RetentionNotification {

    private int notificationId;
    private int notificationTime; // In minutes
    private String channelId;
    private String notificationTitle;
    private String notificationText;

    public RetentionNotification(int notificationId, int notificationTime, String channelId, String notificationTitle, String notificationText) {
        this.notificationId = notificationId;
        this.notificationTime = notificationTime;
        this.channelId = channelId;
        this.notificationTitle = notificationTitle;
        this.notificationText = notificationText;
    }

    public int getNotificationId() {
        return notificationId;
    }

    public int getNotificationTime() {
        return notificationTime;
    }

    public String getChannelId() {
        return channelId;
    }

    public String getNotificationTitle() {
        return notificationTitle;
    }

    public String getNotificationText() {
        return notificationText;
    }
}
