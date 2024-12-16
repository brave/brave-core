/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.media.ui;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.components.browser_ui.media.BraveMediaSessionHelper;
import org.chromium.components.browser_ui.media.MediaNotificationInfo;
import org.chromium.components.browser_ui.media.MediaNotificationManager;

public class BraveMediaSessionTabHelper extends MediaSessionTabHelper {
    /** Will be deleted in bytecode, value from the parent class will be used instead. */
    private Tab mTab;

    BraveMediaSessionTabHelper(Tab tab) {
        super(tab);
    }

    @Override
    public MediaNotificationInfo.Builder createMediaNotificationInfoBuilder() {
        if (!BraveMediaSessionHelper.isBraveTalk(mTab.getWebContents())) {
            return super.createMediaNotificationInfoBuilder();
        }

        return new MediaNotificationInfo.Builder()
                .setInstanceId(mTab.getId())
                .setId(R.id.media_playback_mic_notification);
    }

    @Override
    public void hideMediaNotification() {
        if (!BraveMediaSessionHelper.isBraveTalk(mTab.getWebContents())) {
            super.hideMediaNotification();
            return;
        }
        MediaNotificationManager.hide(mTab.getId(), R.id.media_playback_mic_notification);
    }

    @Override
    public void activateAndroidMediaSession() {
        if (!BraveMediaSessionHelper.isBraveTalk(mTab.getWebContents())) {
            super.activateAndroidMediaSession();
            return;
        }
        MediaNotificationManager.activateAndroidMediaSession(
                mTab.getId(), R.id.media_playback_mic_notification);
    }
}
