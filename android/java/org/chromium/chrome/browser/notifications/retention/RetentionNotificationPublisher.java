/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.notifications.retention;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

import org.chromium.base.ApplicationStatus;
import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.base.task.AsyncTask;
import org.chromium.chrome.browser.BraveRewardsNativeWorker;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.onboarding.OnboardingPrefManager;
import org.chromium.chrome.browser.set_default_browser.BraveSetDefaultBrowserUtils;
import org.chromium.chrome.browser.tab.TabLaunchType;
import org.chromium.components.embedder_support.util.UrlConstants;
import org.chromium.components.embedder_support.util.UrlUtilities;

public class RetentionNotificationPublisher extends BroadcastReceiver {
    private static final String NOTIFICATION_CHANNEL_NAME = "brave";
    public static final String RETENTION_NOTIFICATION_ACTION = "retention_notification_action";

    @Override
    public void onReceive(Context context, Intent intent) {
        try {
            String action = intent.getAction();
            String notificationType =
                    intent.getStringExtra(RetentionNotificationUtil.NOTIFICATION_TYPE);
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
                            if (braveActivity.getActivityTab() != null
                                    && braveActivity.getActivityTab().getUrl().getSpec() != null
                                    && !UrlUtilities.isNtpUrl(
                                            braveActivity.getActivityTab().getUrl().getSpec())) {
                                braveActivity.getTabCreator(false).launchUrl(
                                        UrlConstants.NTP_URL, TabLaunchType.FROM_CHROME_UI);
                            }
                            break;
                        case RetentionNotificationUtil.DAY_10:
                        case RetentionNotificationUtil.DAY_30:
                        case RetentionNotificationUtil.DAY_35:
                            braveActivity.openRewardsPanel();
                            break;
                        case RetentionNotificationUtil.DORMANT_USERS_DAY_14:
                        case RetentionNotificationUtil.DORMANT_USERS_DAY_25:
                        case RetentionNotificationUtil.DORMANT_USERS_DAY_40:
                            if (System.currentTimeMillis()
                                    > OnboardingPrefManager.getInstance()
                                              .getDormantUsersNotificationTime(notificationType)) {
                                braveActivity.showDormantUsersEngagementDialog(notificationType);
                            } else {
                                RetentionNotificationUtil.scheduleNotificationWithTime(context,
                                        notificationType,
                                        OnboardingPrefManager.getInstance()
                                                .getDormantUsersNotificationTime(notificationType));
                            }
                            break;
                        case RetentionNotificationUtil.DEFAULT_BROWSER_1:
                        case RetentionNotificationUtil.DEFAULT_BROWSER_2:
                        case RetentionNotificationUtil.DEFAULT_BROWSER_3:
                            if (!BraveSetDefaultBrowserUtils.isBraveSetAsDefaultBrowser(
                                        braveActivity)
                                    && !BraveSetDefaultBrowserUtils.isBraveDefaultDontAsk()) {
                                BraveSetDefaultBrowserUtils.showBraveSetDefaultBrowserDialog(
                                        braveActivity, false);
                            }
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
                    createNotification(context, intent);
                    break;
                case RetentionNotificationUtil.DEFAULT_BROWSER_1:
                case RetentionNotificationUtil.DEFAULT_BROWSER_2:
                case RetentionNotificationUtil.DEFAULT_BROWSER_3:
                    if (!BraveSetDefaultBrowserUtils.isBraveSetAsDefaultBrowser(context)
                            && !BraveSetDefaultBrowserUtils.isBraveDefaultDontAsk()) {
                        createNotification(context, intent);
                    }
                    break;
                case RetentionNotificationUtil.DAY_10:
                case RetentionNotificationUtil.DAY_30:
                case RetentionNotificationUtil.DAY_35:
                    // Can't check for rewards code in background
                    try {
                        BraveRewardsNativeWorker rewardsNativeWorker =
                                BraveRewardsNativeWorker.getInstance();
                            if (braveActivity != null
                                    && rewardsNativeWorker != null
                                    && !rewardsNativeWorker.isRewardsEnabled()
                                    && rewardsNativeWorker.isSupported()) {
                                createNotification(context, intent);
                        }
                    } catch (IllegalStateException exc) {
                        // We can receive 'Browser hasn't finished initialization yet!' if
                        // Profile.getLastUsedRegularProfile is called too early. Just ignore it,
                        // it's better comparing to crashing
                    }
                    break;
                case RetentionNotificationUtil.EVERY_SUNDAY:
                    if (OnboardingPrefManager.getInstance().isBraveStatsNotificationEnabled()) {
                        createNotification(context, intent);
                    }
                    break;
                case RetentionNotificationUtil.DORMANT_USERS_DAY_14:
                case RetentionNotificationUtil.DORMANT_USERS_DAY_25:
                case RetentionNotificationUtil.DORMANT_USERS_DAY_40:
                    if (System.currentTimeMillis()
                                    > OnboardingPrefManager.getInstance()
                                              .getDormantUsersNotificationTime(notificationType)
                            && !BraveSetDefaultBrowserUtils.isBraveSetAsDefaultBrowser(
                                    braveActivity)
                            && !BraveSetDefaultBrowserUtils.isBraveDefaultDontAsk()) {
                        createNotification(context, intent);
                    } else {
                        RetentionNotificationUtil.scheduleNotificationWithTime(context,
                                notificationType,
                                OnboardingPrefManager.getInstance().getDormantUsersNotificationTime(
                                        notificationType));
                    }
                    break;
                }
            }
        } catch (Exception exc) {
            // There could be uninitialized parts on early stages. Just ignore it the exception,
            // it's better comparing to crashing
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
        if (ApplicationStatus.hasVisibleActivities() || context == null
                || context.getPackageManager() == null) {
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
