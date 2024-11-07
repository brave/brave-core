/**
 * Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.notifications;

import android.app.Notification;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.dialogs.BraveAdsNotificationDialog;
import org.chromium.chrome.browser.notifications.retention.RetentionNotificationPublisher;
import org.chromium.chrome.browser.notifications.retention.RetentionNotificationUtil;
import org.chromium.components.browser_ui.notifications.NotificationManagerProxyImpl;
import org.chromium.components.browser_ui.notifications.NotificationMetadata;
import org.chromium.components.browser_ui.notifications.NotificationWrapper;
import org.chromium.components.browser_ui.notifications.PendingIntentProvider;

import java.util.Locale;

public class BraveOnboardingNotification extends BroadcastReceiver {
    public Context mContext;
    private static final String TAG = "OnboardingNoti";

    private static final int BRAVE_ONBOARDING_NOTIFICATION_ID = -2;
    public static String BRAVE_ONBOARDING_NOTIFICATION_TAG = "brave_onboarding_notification_tag";
    private static String BRAVE_ONBOARDING_ORIGIN_EN = "https://brave.com/my-first-ad/";
    private static String BRAVE_ONBOARDING_ORIGIN_DE = "https://brave.com/de/my-first-ad/";
    private static String BRAVE_ONBOARDING_ORIGIN_FR = "https://brave.com/fr/my-first-ad/";
    public static final String DEEP_LINK = "deep_link";
    public static final String USE_CUSTOM_NOTIFICATION = "use_custom_notification";

    private static final String COUNTRY_CODE_DE = "de_DE";
    private static final String COUNTRY_CODE_FR = "fr_FR";

    public static void showOnboardingDialog() {
        try {
            BraveActivity braveActivity = BraveActivity.getBraveActivity();
            BraveAdsNotificationDialog.showNotificationAd(braveActivity,
                    BRAVE_ONBOARDING_NOTIFICATION_TAG, getNotificationUrl(),
                    braveActivity.getString(R.string.brave_ui_brave_rewards),
                    braveActivity.getString(R.string.this_is_your_first_ad));
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "showOnboardingDialog " + e);
        }
    }

    public static void showOnboardingNotification() {
        Context context = ContextUtils.getApplicationContext();
        if (context == null) return;
        NotificationManagerProxyImpl notificationManager =
            new NotificationManagerProxyImpl(context);

        NotificationBuilderBase notificationBuilder =
                new BraveNotificationBuilder(context)
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
        return PendingIntentProvider.getBroadcast(
                context, 0, intent, PendingIntent.FLAG_UPDATE_CURRENT, true);
    }

    @Override
    public void onReceive(Context context, Intent intent) {
        String action = intent.getAction();
        try {
            BraveActivity braveActivity = BraveActivity.getBraveActivity();
            if (action != null && action.equals(DEEP_LINK)) {
                Intent launchIntent =
                        new Intent(Intent.ACTION_VIEW, Uri.parse(getNotificationUrl()));
                launchIntent.setPackage(context.getPackageName());
                launchIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                context.startActivity(launchIntent);
            } else {
                if (intent.getBooleanExtra(USE_CUSTOM_NOTIFICATION, false)) {
                    showOnboardingDialog();
                } else {
                    showOnboardingNotification();
                }
                braveActivity.hideRewardsOnboardingIcon();
            }
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "onReceive " + e);

            if (action != null && action.equals(DEEP_LINK)) {
                intent.putExtra(RetentionNotificationUtil.NOTIFICATION_TYPE,
                        RetentionNotificationUtil.DAY_10);
                RetentionNotificationPublisher.backgroundNotificationAction(context, intent);
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
