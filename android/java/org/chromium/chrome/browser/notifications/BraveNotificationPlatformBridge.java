/**
 * Copyright (c) 2019 The Brave Authors. All rights reserved. This Source Code Form is subject to
 * the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with
 * this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
package org.chromium.chrome.browser.notifications;

import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;

import androidx.annotation.VisibleForTesting;

import org.chromium.base.ApplicationStatus;
import org.chromium.base.ContextUtils;
import org.chromium.chrome.browser.notifications.NotificationPlatformBridge.NotificationIdentifyingAttributes;

public class BraveNotificationPlatformBridge {
    private static final int[] EMPTY_VIBRATION_PATTERN = new int[0];

    public static boolean dispatchNotificationEvent(Intent intent) {
        if (NotificationPlatformBridge.dispatchNotificationEvent(intent)) {
            @NotificationType
            int notificationType =
                    intent.getIntExtra(
                            NotificationConstants.EXTRA_NOTIFICATION_TYPE,
                            NotificationType.WEB_PERSISTENT);
            if (notificationType == NotificationType.BRAVE_ADS
                    && NotificationConstants.ACTION_CLICK_NOTIFICATION.equals(intent.getAction())) {
                bringToForeground();
            }
            return true;
        }

        return false;
    }

    private static void bringToForeground() {
        if (ApplicationStatus.hasVisibleActivities()) {
            return;
        }
        Context context = ContextUtils.getApplicationContext();
        Intent launchIntent =
                context.getPackageManager().getLaunchIntentForPackage(context.getPackageName());
        if (launchIntent != null) {
            launchIntent.setFlags(
                    Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_RESET_TASK_IF_NEEDED);
            context.startActivity(launchIntent);
        }
    }

    public static NotificationBuilderBase prepareNotificationBuilder(
            NotificationIdentifyingAttributes identifyingAttributes,
            boolean vibrateEnabled,
            String title,
            String body,
            Bitmap image,
            Bitmap icon,
            Bitmap badge,
            int[] vibrationPattern,
            long timestamp,
            boolean renotify,
            boolean silent,
            ActionInfo[] actions) {
        if (identifyingAttributes.notificationType == NotificationType.BRAVE_ADS) {
            vibrationPattern = EMPTY_VIBRATION_PATTERN;
        }

        NotificationBuilderBase result =
                NotificationPlatformBridge.prepareNotificationBuilder(
                        identifyingAttributes,
                        vibrateEnabled,
                        title,
                        body,
                        image,
                        icon,
                        badge,
                        vibrationPattern,
                        timestamp,
                        renotify,
                        silent,
                        actions);

        assert result instanceof BraveNotificationBuilder
                : "Bytecode changes for BraveNotificationBuilder were not applied!";
        if (result instanceof BraveNotificationBuilder) {
            ((BraveNotificationBuilder) result)
                    .setIsBraveNotification(
                            identifyingAttributes.notificationType == NotificationType.BRAVE_ADS);
        }

        return result;
    }

    @VisibleForTesting
    public static Class<ActionInfo[]> getActionInfoArrayClass() {
        return ActionInfo[].class;
    }
}
