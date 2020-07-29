/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.notifications.retention;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;

import org.chromium.base.Log;
import org.chromium.base.ApplicationStatus;
import org.chromium.chrome.browser.BraveActivity;

public class RetentionNotificationPublisher extends BroadcastReceiver {
    private static final String NOTIFICATION_CHANNEL_NAME = "brave";
    public static final String RETENTION_NOTIFICATION_ACTION = "retention_notification_action";

    public void onReceive(Context context, Intent intent) {
        String action = intent.getAction();
        if (action != null && action.equals(RETENTION_NOTIFICATION_ACTION)) {
            BraveActivity braveActivity = BraveActivity.getBraveActivity();
            if (braveActivity != null) {
                braveActivity.showOnboarding();
            } else {
                backgroundNotificationAction(context, intent);
            }
        } else {
            createNotification(context, intent);
        }
    }

    private void createNotification(Context context, Intent intent) {
        String notificationType = intent.getStringExtra(RetentionNotificationUtil.NOTIFICATION_TYPE);
        RetentionNotification retentionNotification = RetentionNotificationUtil.getNotificationObject(notificationType);
        NotificationManager notificationManager = (NotificationManager)context.getSystemService(Context. NOTIFICATION_SERVICE);
        Log.e("NTP", "Notification : " + notificationType);
        Notification notification = RetentionNotificationUtil.getNotification(context, notificationType);
        if (android.os.Build.VERSION. SDK_INT >= android.os.Build.VERSION_CODES. O ) {
            int importance = NotificationManager.IMPORTANCE_HIGH ;
            NotificationChannel notificationChannel = new NotificationChannel(retentionNotification.getChannelId() , NOTIFICATION_CHANNEL_NAME, importance) ;
            assert notificationManager != null;
            notificationManager.createNotificationChannel(notificationChannel);
        }
        assert notificationManager != null;
        notificationManager.notify(retentionNotification.getNotificationId(), notification) ;
    }

    private void backgroundNotificationAction(Context context, Intent intent) {
        String notificationType = intent.getStringExtra(RetentionNotificationUtil.NOTIFICATION_TYPE);
        RetentionNotification retentionNotification = RetentionNotificationUtil.getNotificationObject(notificationType);
        if (ApplicationStatus.hasVisibleActivities()) {
            return;
        }
        Intent launchIntent =
            context.getPackageManager().getLaunchIntentForPackage(context.getPackageName());
        if (launchIntent != null) {
            launchIntent.setFlags(
                Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_RESET_TASK_IF_NEEDED);
            launchIntent.putExtra(RetentionNotificationUtil.NOTIFICATION_TYPE, notificationType);
            context.startActivity(launchIntent);
        }
    }
}