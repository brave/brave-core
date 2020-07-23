/**
 * Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.notifications;

import android.app.Notification;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.notifications.BraveAdsNotificationBuilder;
import org.chromium.chrome.browser.notifications.NotificationBuilderBase;
import org.chromium.chrome.browser.notifications.NotificationUmaTracker;
import org.chromium.chrome.browser.notifications.retention.RetentionNotificationUtil;
import org.chromium.chrome.browser.notifications.retention.RetentionNotificationPublisher;
import org.chromium.components.browser_ui.notifications.NotificationManagerProxyImpl;
import org.chromium.components.browser_ui.notifications.NotificationMetadata;
import org.chromium.components.browser_ui.notifications.NotificationWrapper;
import org.chromium.components.browser_ui.notifications.PendingIntentProvider;
import org.chromium.chrome.browser.BraveActivity;

import java.util.Locale;

public class BraveOnboardingNotification extends BroadcastReceiver {
    public Context mContext;
    private Intent mIntent;

    private static final int BRAVE_ONBOARDING_NOTIFICATION_ID = -2;
    private static String BRAVE_ONBOARDING_NOTIFICATION_TAG = "brave_onboarding_notification_tag";
    private static String BRAVE_ONBOARDING_ORIGIN_EN = "https://brave.com/my-first-ad/";
    private static String BRAVE_ONBOARDING_ORIGIN_DE = "https://brave.com/de/my-first-ad/";
    private static String BRAVE_ONBOARDING_ORIGIN_FR = "https://brave.com/fr/my-first-ad/";
    public static final String DEEP_LINK = "deep_link";

    private static final String COUNTRY_CODE_DE = "de_DE";
    private static final String COUNTRY_CODE_FR = "fr_FR";

    public static void showOnboardingNotification(Context context) {
        if (context == null) return;
        NotificationManagerProxyImpl notificationManager =
            new NotificationManagerProxyImpl(context);

        NotificationBuilderBase notificationBuilder =
            new BraveAdsNotificationBuilder(context)
        .setTitle(context.getString(R.string.brave_ui_brave_rewards))
        .setBody(context.getString(R.string.this_is_your_first_ad))
        .setSmallIconId(R.drawable.ic_chrome)
        .setPriority(Notification.PRIORITY_HIGH)
        .setDefaults(Notification.DEFAULT_ALL)
        .setContentIntent(getDeepLinkIntent(context))
        .setOrigin(getNotificationUrl());

        NotificationWrapper notification = notificationBuilder.build(new NotificationMetadata(
                                              NotificationUmaTracker.SystemNotificationType
                                              .UNKNOWN /* Underlying code doesn't track UNKNOWN */,
                                              BRAVE_ONBOARDING_NOTIFICATION_TAG /* notificationTag */,
                                              BRAVE_ONBOARDING_NOTIFICATION_ID /* notificationId */
                                          ));
        notificationManager.notify(notification);
    }

    public static PendingIntentProvider getDeepLinkIntent(Context context) {
        Intent intent = new Intent(context, BraveOnboardingNotification.class);
        intent.setAction(DEEP_LINK);
        return new PendingIntentProvider(
                   PendingIntent.getBroadcast(context, 0, intent, PendingIntent.FLAG_UPDATE_CURRENT),
                   0);
    }

    @Override
    public void onReceive(Context context, Intent intent) {
        String action = intent.getAction();
        BraveActivity braveActivity = BraveActivity.getBraveActivity();
        if (action != null && action.equals(DEEP_LINK)) {
            if (braveActivity != null) {
                braveActivity.openRewardsPanel();
                Intent launchIntent = new Intent(Intent.ACTION_MAIN);
                launchIntent.setPackage(context.getPackageName());
                launchIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                context.startActivity(launchIntent);
            } else {
                intent.putExtra(RetentionNotificationUtil.NOTIFICATION_TYPE, RetentionNotificationUtil.DAY_10);
                RetentionNotificationPublisher.backgroundNotificationAction(context, intent);
            }
        } else {
            showOnboardingNotification(context);
            if (braveActivity != null) {
                braveActivity.hideRewardsOnboardingIcon();
            }
        }
    }

    private static String getNotificationUrl() {
        Locale locale = Locale.getDefault();
        switch (locale.toString()) {
        case COUNTRY_CODE_DE:
            return BRAVE_ONBOARDING_ORIGIN_DE;
        case COUNTRY_CODE_FR:
            return BRAVE_ONBOARDING_ORIGIN_FR;
        default:
            return BRAVE_ONBOARDING_ORIGIN_EN;
        }
    }

    public static void cancelOnboardingNotification(Context context) {
        NotificationManagerProxyImpl notificationManager =
            new NotificationManagerProxyImpl(context);
        notificationManager.cancel(
            BRAVE_ONBOARDING_NOTIFICATION_TAG, BRAVE_ONBOARDING_NOTIFICATION_ID);
    }
}
