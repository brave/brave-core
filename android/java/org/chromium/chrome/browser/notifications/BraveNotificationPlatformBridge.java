/**
 * Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.notifications;

import android.app.Notification;
import android.content.Context;
import android.graphics.Bitmap;

import org.chromium.base.annotations.CalledByNative;

public class BraveNotificationPlatformBridge extends NotificationPlatformBridge {
    private @NotificationType int mNotificationType;
    private static BraveNotificationPlatformBridge sInstance;

    @CalledByNative
    private static BraveNotificationPlatformBridge create(long nativeNotificationPlatformBridge) {
        if (sInstance != null) {
            throw new IllegalStateException(
                "There must only be a single NotificationPlatformBridge.");
        }

        sInstance = new BraveNotificationPlatformBridge(nativeNotificationPlatformBridge);
        return sInstance;
    }

    private BraveNotificationPlatformBridge(long nativeNotificationPlatformBridge) {
        super(nativeNotificationPlatformBridge);
    }

    @Override
    protected NotificationBuilderBase prepareNotificationBuilder(String notificationId,
            @NotificationType int notificationType, String origin, String scopeUrl,
            String profileId, boolean incognito, String title, String body, Bitmap image,
            Bitmap icon, Bitmap badge, int[] vibrationPattern, long timestamp, boolean renotify,
            boolean silent, ActionInfo[] actions, String webApkPackage) {
        mNotificationType = notificationType;

        return super.prepareNotificationBuilder(notificationId, notificationType, origin, scopeUrl,
                profileId, incognito, title, body, image, icon, badge, vibrationPattern, timestamp,
                renotify, silent, actions, webApkPackage);
    }

    @Override
    protected NotificationBuilderBase createNotificationBuilder(Context context, boolean hasImage) {
        if (mNotificationType == NotificationType.BRAVE_ADS) {
            // TODO(jocelyn): Remove setPriority here since we already set the
            // importance of Ads notification channel to IMPORTANCE_HIGH?
            return new BraveAdsNotificationBuilder(context).setPriority(Notification.PRIORITY_HIGH);
        }

        return super.createNotificationBuilder(context, hasImage);
    }
}
