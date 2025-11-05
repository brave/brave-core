/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.media.ui;

import static org.chromium.build.NullUtil.assumeNonNull;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.components.browser_ui.media.BraveMediaSessionHelper;
import org.chromium.components.browser_ui.media.MediaNotificationInfo;
import org.chromium.components.browser_ui.media.MediaNotificationManager;

@NullMarked
public class BraveMediaSessionTabHelper extends MediaSessionTabHelper {
    /** Will be deleted in bytecode, value from the parent class will be used instead. */
    private @Nullable Tab mTab;

    BraveMediaSessionTabHelper(Tab tab) {
        super(tab);
    }

    @Override
    public MediaNotificationInfo.Builder createMediaNotificationInfoBuilder() {
        if (!BraveMediaSessionHelper.isBraveTalk(assumeNonNull(mTab).getWebContents())) {
            return super.createMediaNotificationInfoBuilder();
        }

        return new MediaNotificationInfo.Builder()
                .setInstanceId(assumeNonNull(mTab).getId())
                .setId(R.id.media_playback_mic_notification);
    }

    @Override
    public void hideMediaNotification() {
        if (mTab == null) return; // Return early if onDestroy was already called.

        if (!BraveMediaSessionHelper.isBraveTalk(mTab.getWebContents())) {
            super.hideMediaNotification();
            return;
        }
        MediaNotificationManager.hide(mTab.getId(), R.id.media_playback_mic_notification);
    }

    @Override
    public void activateAndroidMediaSession() {
        if (mTab == null) return; // Return early if onDestroy was already called.

        if (!BraveMediaSessionHelper.isBraveTalk(mTab.getWebContents())) {
            super.activateAndroidMediaSession();
            return;
        }
        MediaNotificationManager.activateAndroidMediaSession(
                mTab.getId(), R.id.media_playback_mic_notification);
    }
}
