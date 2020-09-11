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

import org.chromium.base.ApplicationStatus;
import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.base.task.AsyncTask;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.BraveAdsNativeHelper;
import org.chromium.chrome.browser.BraveFeatureList;
import org.chromium.chrome.browser.brave_stats.BraveStatsUtil;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.notifications.BraveSetDefaultBrowserNotificationService;
import org.chromium.chrome.browser.ntp.NewTabPage;
import org.chromium.chrome.browser.onboarding.OnboardingPrefManager;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.components.user_prefs.UserPrefs;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.tab.TabLaunchType;
import org.chromium.components.embedder_support.util.UrlConstants;

public class RetentionNotificationPublisher extends BroadcastReceiver {
    private static final String NOTIFICATION_CHANNEL_NAME = "brave";
    public static final String RETENTION_NOTIFICATION_ACTION = "retention_notification_action";

    @Override
    public void onReceive(Context context, Intent intent) {
        String action = intent.getAction();
        String notificationType = intent.getStringExtra(RetentionNotificationUtil.NOTIFICATION_TYPE);
        BraveActivity braveActivity = BraveActivity.getBraveActivity();
        if (action != null && action.equals(RETENTION_NOTIFICATION_ACTION)) {
            if (braveActivity != null) {
                Intent launchIntent = new Intent(Intent.ACTION_MAIN);
                launchIntent.setPackage(context.getPackageName());
                launchIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                context.startActivity(launchIntent);
                switch (notificationType) {
                case RetentionNotificationUtil.HOUR_3:
                case RetentionNotificationUtil.HOUR_24:
                case RetentionNotificationUtil.EVERY_SUNDAY:
                    braveActivity.checkForBraveStats();
                    break;
                case RetentionNotificationUtil.DAY_6:
                case RetentionNotificationUtil.BRAVE_STATS_ADS_TRACKERS:
                case RetentionNotificationUtil.BRAVE_STATS_DATA:
                case RetentionNotificationUtil.BRAVE_STATS_TIME:
                    if (!NewTabPage.isNTPUrl(braveActivity.getActivityTab().getUrlString())) {
                        braveActivity.getTabCreator(false).launchUrl(UrlConstants.NTP_URL, TabLaunchType.FROM_CHROME_UI);
                    }
                    break;
                case RetentionNotificationUtil.DAY_10:
                case RetentionNotificationUtil.DAY_30:
                case RetentionNotificationUtil.DAY_35:
                    braveActivity.openRewardsPanel();
                    break;
                }
            } else {
                backgroundNotificationAction(context, intent);
            }
        } else {
            switch (notificationType) {
            case RetentionNotificationUtil.HOUR_3:
            case RetentionNotificationUtil.HOUR_24:
            case RetentionNotificationUtil.DAY_6:
            case RetentionNotificationUtil.BRAVE_STATS_ADS_TRACKERS:
            case RetentionNotificationUtil.BRAVE_STATS_DATA:
            case RetentionNotificationUtil.BRAVE_STATS_TIME:
                createNotification(context, intent);
                break;
            case RetentionNotificationUtil.DEFAULT_BROWSER_1:
            case RetentionNotificationUtil.DEFAULT_BROWSER_2:
            case RetentionNotificationUtil.DEFAULT_BROWSER_3:
                if (!BraveSetDefaultBrowserNotificationService.isBraveSetAsDefaultBrowser(context)) {
                    createNotification(context, intent);
                }
                break;
            case RetentionNotificationUtil.DAY_10:
            case RetentionNotificationUtil.DAY_30:
            case RetentionNotificationUtil.DAY_35:
                // Can't check for rewards code in background
                if (braveActivity != null
                        && (ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_REWARDS) && !UserPrefs.get(Profile.getLastUsedRegularProfile()).getBoolean(BravePref.ENABLED))
                        && !BraveAdsNativeHelper.nativeIsBraveAdsEnabled(Profile.getLastUsedRegularProfile())) {
                    createNotification(context, intent);
                }
                break;
            case RetentionNotificationUtil.EVERY_SUNDAY:
                if (OnboardingPrefManager.getInstance().isBraveStatsNotificationEnabled()) {
                    createNotification(context, intent);
                }
                break;
            }
        }
    }

    private void createNotification(Context context, Intent intent) {
        final String notificationType =
                intent.getStringExtra(RetentionNotificationUtil.NOTIFICATION_TYPE);
        final RetentionNotification retentionNotification =
                RetentionNotificationUtil.getNotificationObject(notificationType);
        new AsyncTask<Void>() {
            String notificationText = "";
            @Override
            protected Void doInBackground() {
                notificationText =
                        RetentionNotificationUtil.getNotificationText(context, notificationType);
                return null;
            }

            @Override
            protected void onPostExecute(Void result) {
                assert ThreadUtils.runningOnUiThread();
                if (isCancelled()) return;
                NotificationManager notificationManager =
                        (NotificationManager) context.getSystemService(
                                Context.NOTIFICATION_SERVICE);
                Log.e("NTP", "Notification : " + notificationType);
                Notification notification = RetentionNotificationUtil.getNotification(
                        context, notificationType, notificationText);
                if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.O) {
                    int importance = NotificationManager.IMPORTANCE_HIGH;
                    NotificationChannel notificationChannel =
                            new NotificationChannel(retentionNotification.getChannelId(),
                                    NOTIFICATION_CHANNEL_NAME, importance);
                    assert notificationManager != null;
                    notificationManager.createNotificationChannel(notificationChannel);
                }
                assert notificationManager != null;
                notificationManager.notify(retentionNotification.getNotificationId(), notification);
            }
        }.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    public static void backgroundNotificationAction(Context context, Intent intent) {
        String notificationType = intent.getStringExtra(RetentionNotificationUtil.NOTIFICATION_TYPE);
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
