/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.notifications;

import android.app.Notification;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;

import org.junit.Assert;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RuntimeEnvironment;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.browser.notifications.channels.ChromeChannelDefinitions;
import org.chromium.components.browser_ui.notifications.NotificationMetadata;
import org.chromium.components.browser_ui.notifications.PendingIntentProvider;

@RunWith(BaseRobolectricTestRunner.class)
public class BraveNotificationBuilderTest {
    @Test
    public void webNotificationRetainsActions() {
        Notification notification = buildNotification(/* isBraveAd= */ false);

        Assert.assertNotNull(notification.actions);
        Assert.assertEquals(3, notification.actions.length);
        Assert.assertEquals("Accept", notification.actions[0].title.toString());
        Assert.assertEquals("Reply", notification.actions[1].title.toString());
        Assert.assertNotNull(notification.actions[1].getRemoteInputs());
        Assert.assertEquals("Site settings", notification.actions[2].title.toString());
    }

    @Test
    public void braveAdsNotificationUsesCustomLayoutWithoutWebActions() {
        Notification notification = buildNotification(/* isBraveAd= */ true);

        Assert.assertTrue(notification.actions == null || notification.actions.length == 0);
        Assert.assertNotNull(notification.bigContentView);
        Assert.assertEquals(
                R.layout.web_notification_big_brave_ads, notification.bigContentView.getLayoutId());
    }

    private Notification buildNotification(boolean isBraveAd) {
        Context context = RuntimeEnvironment.getApplication();
        BraveNotificationBuilder builder = new BraveNotificationBuilder(context);
        builder.setTitle("Title")
                .setBody("Body")
                .setOrigin("https://example.test")
                .setChannelId(ChromeChannelDefinitions.ChannelId.SITES)
                .setSmallIconId(R.drawable.ic_chrome)
                .addButtonAction(null, "Accept", createIntent(context, "accept"))
                .addTextAction(null, "Reply", createIntent(context, "reply"), "Message")
                .addSettingsAction(
                        /* iconId= */ 0,
                        "Site settings",
                        createIntent(context, "settings"),
                        /* umaActionType= */ 0);

        // The platform bridge sets the notification type after adding actions.
        builder.setIsBraveNotification(isBraveAd);
        NotificationMetadata metadata =
                new NotificationMetadata(
                        NotificationUmaTracker.SystemNotificationType.SITES, "test", 1);
        return builder.build(metadata).getNotification();
    }

    private static PendingIntentProvider createIntent(Context context, String action) {
        return PendingIntentProvider.getBroadcast(
                context,
                /* requestCode= */ 0,
                new Intent("BraveNotificationBuilderTest." + action),
                PendingIntent.FLAG_UPDATE_CURRENT);
    }
}
