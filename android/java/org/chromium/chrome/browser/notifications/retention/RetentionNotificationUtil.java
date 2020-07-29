/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.notifications.retention;

import android.app.AlarmManager;
import android.app.Notification;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;

import androidx.core.app.NotificationCompat;

import org.chromium.chrome.R;

import org.chromium.base.Log;
import org.chromium.chrome.browser.onboarding.OnboardingPrefManager;
import org.chromium.chrome.browser.local_database.DatabaseHelper;

import java.util.Calendar;
import java.util.Date;
import java.util.HashMap;
import java.util.Map;

public class RetentionNotificationUtil {
    public static String NOTIFICATION_TYPE = "notification_type";

    public static final String HOUR_3 = "hour_3";
    public static final String HOUR_24 = "hour_24";
    public static final String HOUR_48 = "hour_48";
    public static final String EVERY_SUNDAY = "every_sunday";

    private static Map<String, RetentionNotification> mNotificationMap = new HashMap<String, RetentionNotification>() {
        {
            put(HOUR_3, new RetentionNotification(3, 1, "Brave Stats", "Brave Stats"));
            put(HOUR_24, new RetentionNotification(24, 24 * 60, "Brave Stats", "Brave Stats"));
            put(HOUR_48, new RetentionNotification(48, 48 * 60, "Brave Stats", "Brave Stats"));
            put(EVERY_SUNDAY, new RetentionNotification(7, -1, "Brave Stats", "Brave Stats"));
        }
    };

    public static RetentionNotification getNotificationObject(String notificationType) {
        return mNotificationMap.get(notificationType);
    }

    public static Notification getNotification(Context context, String notificationType) {
        RetentionNotification retentionNotification = getNotificationObject(notificationType);
        String notificationText = getNotificationText(notificationType);
        Log.e("NTP", notificationText);
        NotificationCompat.Builder builder = new NotificationCompat.Builder(context, retentionNotification.getChannelId());
        builder.setContentTitle(retentionNotification.getNotificationTitle());
        builder.setContentText(notificationText);
        builder.setStyle(new NotificationCompat.BigTextStyle().bigText(notificationText));
        builder.setSmallIcon(R.drawable.ic_chrome);
        builder.setAutoCancel(true);
        builder.setContentIntent(getRetentionNotificationActionIntent(context, notificationType));
        return builder.build();
    }

    private static String getNotificationText(String notificationType) {
        DatabaseHelper mDatabaseHelper = DatabaseHelper.getInstance();
        switch (notificationType) {
        case HOUR_3:
            if (OnboardingPrefManager.getInstance().isBraveStatsEnabled()) {
                long trackerAdsCount = mDatabaseHelper.getAllStats().size();
                if (trackerAdsCount >= 5) {
                    return "You blocked " + trackerAdsCount + "trackers & ads with Brave browser in the past 3 hours.";
                } else {
                    return "Browse up to 6x faster on major news sites in Brave by blocking trackers & ads.";
                }
            } else {
                return "Turn on Brave Stats to see how many trackers & ads have been blocked in the past 3 hours.";
            }
        case HOUR_24:
            if (OnboardingPrefManager.getInstance().isBraveStatsEnabled()) {
                return "You saved Xkb data with Brave browser.";
            } else {
                return "Turn on Brave Stats to see how much data you've saved";
            }
        }
        return "";
    }

    private static PendingIntent getRetentionNotificationActionIntent(Context context, String notificationType) {
        Intent intent = new Intent(context, RetentionNotificationPublisher.class);
        intent.setAction(RetentionNotificationPublisher.RETENTION_NOTIFICATION_ACTION);
        intent.putExtra(NOTIFICATION_TYPE, notificationType);
        return PendingIntent.getBroadcast(context, getNotificationObject(notificationType).getNotificationId(), intent, PendingIntent.FLAG_UPDATE_CURRENT);
    }

    public static void scheduleNotification(Context context, String notificationType) {
        RetentionNotification retentionNotification = getNotificationObject(notificationType);
        Intent notificationIntent = new Intent(context, RetentionNotificationPublisher.class);
        notificationIntent.putExtra(NOTIFICATION_TYPE, notificationType);
        PendingIntent pendingIntent = PendingIntent.getBroadcast(context, retentionNotification.getNotificationId(), notificationIntent, 0);
        Calendar calendar = Calendar.getInstance();
        calendar.setTimeInMillis(System.currentTimeMillis());
        calendar.add(Calendar.MINUTE, retentionNotification.getNotificationTime());

        Date date = calendar.getTime();
        AlarmManager alarmManager = (AlarmManager) context.getSystemService(Context.ALARM_SERVICE);
        assert alarmManager != null;
        alarmManager.setExact(AlarmManager.RTC_WAKEUP, date.getTime(), pendingIntent);
    }

    public static void scheduleNotificationForEverySunday(Context context, String notificationType) {
        RetentionNotification retentionNotification = getNotificationObject(notificationType);
        Calendar currentDate = Calendar.getInstance();
        while (currentDate.get(Calendar.DAY_OF_WEEK) != Calendar.SUNDAY) {
            currentDate.add(Calendar.DATE, 1);
        }
        currentDate.set(Calendar.HOUR_OF_DAY, 9);
        currentDate.set(Calendar.MINUTE, 45);
        currentDate.set(Calendar.SECOND, 0);
        currentDate.set(Calendar.MILLISECOND, 0);

        Intent notificationIntent = new Intent(context, RetentionNotificationPublisher.class);
        notificationIntent.putExtra(NOTIFICATION_TYPE, notificationType);
        PendingIntent pendingIntent = PendingIntent.getBroadcast(context, retentionNotification.getNotificationId(), notificationIntent, 0);
        AlarmManager am = (AlarmManager) context.getSystemService(Context.ALARM_SERVICE);
        assert am != null;
        am.setRepeating(AlarmManager.RTC_WAKEUP, currentDate.getTimeInMillis(), AlarmManager.INTERVAL_DAY * 7, pendingIntent);
    }
}
