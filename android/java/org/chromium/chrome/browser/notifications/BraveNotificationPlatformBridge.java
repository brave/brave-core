/**
 * Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.notifications;

import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;

import org.jni_zero.CalledByNative;

import org.chromium.base.ApplicationStatus;
import org.chromium.base.ContextUtils;

public class BraveNotificationPlatformBridge extends NotificationPlatformBridge {
    private static final int[] EMPTY_VIBRATION_PATTERN = new int[0];

    private @NotificationType int mNotificationType;

    @CalledByNative
    private static BraveNotificationPlatformBridge create(long nativeNotificationPlatformBridge) {
        if (sInstance != null) {
            throw new IllegalStateException(
                "There must only be a single NotificationPlatformBridge.");
        }

        sInstance = new BraveNotificationPlatformBridge(nativeNotificationPlatformBridge);
        return (BraveNotificationPlatformBridge) sInstance;
    }

    private BraveNotificationPlatformBridge(long nativeNotificationPlatformBridge) {
        super(nativeNotificationPlatformBridge);
    }

    static boolean dispatchNotificationEvent(Intent intent) {
        if (NotificationPlatformBridge.dispatchNotificationEvent(intent)) {
            @NotificationType
            int notificationType = intent.getIntExtra(
                    NotificationConstants.EXTRA_NOTIFICATION_TYPE, NotificationType.WEB_PERSISTENT);
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

    @Override
    protected NotificationBuilderBase prepareNotificationBuilder(
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
        mNotificationType = identifyingAttributes.notificationType;

        if (mNotificationType == NotificationType.BRAVE_ADS) {
            vibrationPattern = EMPTY_VIBRATION_PATTERN;
        }

        NotificationBuilderBase result =
                super.prepareNotificationBuilder(
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
                    .setIsBraveNotification(mNotificationType == NotificationType.BRAVE_ADS);
        }

        return result;
    }
}
