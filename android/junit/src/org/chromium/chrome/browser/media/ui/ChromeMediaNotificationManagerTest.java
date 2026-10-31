/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.media.ui;

import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RuntimeEnvironment;

import org.chromium.base.ContextUtils;
import org.chromium.base.FeatureOverrides;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.components.browser_ui.media.MediaNotificationController;
import org.chromium.components.browser_ui.media.MediaNotificationInfo;
import org.chromium.components.browser_ui.media.MediaNotificationListener;
import org.chromium.components.browser_ui.media.MediaNotificationManager;
import org.chromium.components.browser_ui.notifications.ForegroundServiceUtils;
import org.chromium.services.media_session.MediaMetadata;

/** Unit tests for {@link ChromeMediaNotificationManager}. */
@RunWith(BaseRobolectricTestRunner.class)
public class ChromeMediaNotificationManagerTest {
    private static final int NOTIFICATION_ID = R.id.media_playback_mic_notification;

    @Before
    public void setUp() {
        ContextUtils.initApplicationContextForTests(RuntimeEnvironment.application);
        FeatureOverrides.newBuilder()
                .disable(ChromeFeatureList.ALLOW_MULTIPLE_MEDIA_NOTIFICATIONS)
                .applyWithoutOverwrite();
        ForegroundServiceUtils.setInstanceForTesting(mock(ForegroundServiceUtils.class));
    }

    @After
    public void tearDown() {
        MediaNotificationManager.hideForAllTabs(NOTIFICATION_ID);
    }

    // Only BraveMediaNotificationControllerDelegate's constructor registers
    // PlaybackListenerMicService as the foreground service for this notification ID, which is
    // what keeps a Brave Talk call's microphone alive when the app is backgrounded.
    @Test
    public void testShowUsesBraveMediaNotificationControllerDelegate() {
        MediaNotificationInfo notificationInfo =
                new MediaNotificationInfo.Builder()
                        .setMetadata(new MediaMetadata("title", "artist", "album"))
                        .setOrigin("https://talk.brave.com")
                        .setListener(mock(MediaNotificationListener.class))
                        .setId(NOTIFICATION_ID)
                        .setInstanceId(1)
                        .build();

        ChromeMediaNotificationManager.show(notificationInfo);

        MediaNotificationController controller =
                MediaNotificationManager.getControllerByNotificationId(NOTIFICATION_ID);
        assertNotNull(controller);
        assertTrue(controller.mDelegate instanceof BraveMediaNotificationControllerDelegate);
    }
}
