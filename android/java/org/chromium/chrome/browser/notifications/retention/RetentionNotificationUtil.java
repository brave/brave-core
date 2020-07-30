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
import android.util.Pair;

import androidx.core.app.NotificationCompat;

import org.chromium.chrome.R;

import org.chromium.base.Log;
import org.chromium.base.ContextUtils;
import org.chromium.chrome.browser.onboarding.OnboardingPrefManager;
import org.chromium.chrome.browser.local_database.DatabaseHelper;
import org.chromium.chrome.browser.notifications.channels.BraveChannelDefinitions;
import org.chromium.chrome.browser.brave_stats.BraveStatsUtil;
import org.chromium.chrome.browser.ntp.BraveNewTabPageLayout;

import java.util.Calendar;
import java.util.Date;
import java.util.HashMap;
import java.util.Map;

public class RetentionNotificationUtil {
    public static String NOTIFICATION_TYPE = "notification_type";

    public static final String HOUR_3 = "hour_3";
    public static final String HOUR_24 = "hour_24";
    public static final String HOUR_48 = "hour_48";
    public static final String DAY_6 = "day_6";
    public static final String EVERY_SUNDAY = "every_sunday";
    public static final String DAY_10 = "day_10";
    public static final String DAY_30 = "day_30";
    public static final String DAY_35 = "day_35";
    public static final String BRAVE_STATS_ADS_TRACKERS = "brave_stats_ads_trackers";
    public static final String BRAVE_STATS_DATA = "brave_stats_data";
    public static final String BRAVE_STATS_TIME = "brave_stats_time";

    private static Map<String, RetentionNotification> mNotificationMap = new HashMap<String, RetentionNotification>() {
        {
            put(HOUR_3, new RetentionNotification(3, 3 * 60, BraveChannelDefinitions.ChannelId.BRAVE_STATS, ContextUtils.getApplicationContext().getResources().getString(R.string.brave_stats)));
            put(HOUR_24, new RetentionNotification(24, 24 * 60, BraveChannelDefinitions.ChannelId.BRAVE_STATS, ContextUtils.getApplicationContext().getResources().getString(R.string.brave_stats)));
            put(HOUR_48, new RetentionNotification(48, 48 * 60, BraveChannelDefinitions.ChannelId.BRAVE_BROWSER, ContextUtils.getApplicationContext().getResources().getString(R.string.brave_stats)));
            put(DAY_6, new RetentionNotification(6, 6 * 24 * 60, BraveChannelDefinitions.ChannelId.BRAVE_BROWSER, ContextUtils.getApplicationContext().getResources().getString(R.string.brave_stats)));
            put(EVERY_SUNDAY, new RetentionNotification(7, -1, BraveChannelDefinitions.ChannelId.BRAVE_STATS, ContextUtils.getApplicationContext().getResources().getString(R.string.brave_stats)));
            put(DAY_10, new RetentionNotification(10, 10 * 24 * 60, BraveChannelDefinitions.ChannelId.BRAVE_REWARDS, ContextUtils.getApplicationContext().getResources().getString(R.string.brave_ui_brave_rewards)));
            put(DAY_30, new RetentionNotification(30, 30 * 24 * 60, BraveChannelDefinitions.ChannelId.BRAVE_REWARDS, ContextUtils.getApplicationContext().getResources().getString(R.string.brave_ui_brave_rewards)));
            put(DAY_35, new RetentionNotification(35, 35 * 24 * 60, BraveChannelDefinitions.ChannelId.BRAVE_REWARDS, ContextUtils.getApplicationContext().getResources().getString(R.string.brave_ui_brave_rewards)));
            put(BRAVE_STATS_ADS_TRACKERS, new RetentionNotification(14, 60, BraveChannelDefinitions.ChannelId.BRAVE_STATS, ContextUtils.getApplicationContext().getResources().getString(R.string.brave_stats)));
            put(BRAVE_STATS_DATA, new RetentionNotification(15, 60, BraveChannelDefinitions.ChannelId.BRAVE_STATS, ContextUtils.getApplicationContext().getResources().getString(R.string.brave_stats)));
            put(BRAVE_STATS_TIME, new RetentionNotification(16, 60, BraveChannelDefinitions.ChannelId.BRAVE_STATS, ContextUtils.getApplicationContext().getResources().getString(R.string.brave_stats)));
        }
    };

    public static RetentionNotification getNotificationObject(String notificationType) {
        return mNotificationMap.get(notificationType);
    }

    public static Notification getNotification(Context context, String notificationType) {
        RetentionNotification retentionNotification = getNotificationObject(notificationType);
        String notificationText = getNotificationText(context, notificationType);
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

    private static String getNotificationText(Context context, String notificationType) {
        DatabaseHelper mDatabaseHelper = DatabaseHelper.getInstance();
        long totalSavedBandwidth = mDatabaseHelper.getTotalSavedBandwidth();
        long adsTrackersCount = mDatabaseHelper.getAllStats().size();
        long timeSavedCount = adsTrackersCount * BraveNewTabPageLayout.MILLISECONDS_PER_ITEM;

        long adsTrackersCountWeekly = mDatabaseHelper.getAllStatsWithDate(BraveStatsUtil.getCalculatedDate("yyyy-MM-dd", 7), BraveStatsUtil.getCalculatedDate("yyyy-MM-dd", 0)).size();

        Pair<String, String> adsTrackersPair = BraveStatsUtil.getBraveStatsStringFormNumberPair(adsTrackersCount, false);
        Pair<String, String> dataSavedPair = BraveStatsUtil.getBraveStatsStringFormNumberPair(totalSavedBandwidth, true);
        switch (notificationType) {
        case HOUR_3:
            if (OnboardingPrefManager.getInstance().isBraveStatsEnabled()) {
                if (adsTrackersCount >= 5) {
                    return String.format(context.getResources().getString(R.string.notification_hour_3_text_1), adsTrackersCount);
                } else {
                    return context.getResources().getString(R.string.notification_hour_3_text_2);
                }
            } else {
                return context.getResources().getString(R.string.notification_hour_3_text_3);
            }
        case HOUR_24:
            if (OnboardingPrefManager.getInstance().isBraveStatsEnabled()) {
                return String.format(context.getResources().getString(R.string.notification_hour_24_text_1), totalSavedBandwidth);
            } else {
                return context.getResources().getString(R.string.notification_hour_24_text_2);
            }
        case EVERY_SUNDAY:
            return String.format(context.getResources().getString(R.string.notification_weekly_stats), adsTrackersCountWeekly);
        case DAY_6:
            return context.getResources().getString(R.string.notification_marketing);
        case DAY_10:
        case DAY_30:
        case DAY_35:
            return context.getResources().getString(R.string.notification_rewards);
        case BRAVE_STATS_ADS_TRACKERS:
            return context.getResources().getString(R.string.notification_brave_stats_trackers);
        case BRAVE_STATS_DATA:
            return context.getResources().getString(R.string.notification_brave_stats_data);
        case BRAVE_STATS_TIME:
            return context.getResources().getString(R.string.notification_brave_stats_time);
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
