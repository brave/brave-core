/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.media.ui;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.media.AudioManager;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.notifications.NotificationConstants;

public class BraveMediaNotificationControllerDelegate
        extends ChromeMediaNotificationControllerDelegate {
    private static final String TAG = "BraveMediaNotif";

    BraveMediaNotificationControllerDelegate(int id) {
        super(id);
        ChromeMediaNotificationControllerDelegate.sMapNotificationIdToOptions.put(
                PlaybackListenerMicServiceImpl.NOTIFICATION_ID,
                new NotificationOptions(
                        BraveMediaNotificationControllerServices.PlaybackListenerMicService.class,
                        NotificationConstants.GROUP_MEDIA_PLAYBACK));
    }

    private static Context getContext() {
        assert false;
        return null;
    }

    /** Service used to run Brave Talk session */
    public static final class PlaybackListenerMicServiceImpl extends ListenerServiceImpl {
        static final int NOTIFICATION_ID = R.id.media_playback_mic_notification;

        public PlaybackListenerMicServiceImpl() {
            super(NOTIFICATION_ID);
        }

        @Override
        public void onCreate() {
            super.onCreate();
            IntentFilter filter = new IntentFilter(AudioManager.ACTION_AUDIO_BECOMING_NOISY);
            ContextUtils.registerProtectedBroadcastReceiver(
                    getService(), mAudioBecomingNoisyReceiver, filter);
        }

        @Override
        public void onDestroy() {
            getService().unregisterReceiver(mAudioBecomingNoisyReceiver);
            super.onDestroy();
        }

        private BroadcastReceiver mAudioBecomingNoisyReceiver =
                new BroadcastReceiver() {
                    @Override
                    public void onReceive(Context context, Intent intent) {
                        if (!AudioManager.ACTION_AUDIO_BECOMING_NOISY.equals(intent.getAction())) {
                            return;
                        }

                        Intent i =
                                new Intent(
                                        getContext(),
                                        BraveMediaNotificationControllerServices
                                                .PlaybackListenerMicService.class);
                        i.setAction(intent.getAction());
                        try {
                            getContext().startService(i);
                        } catch (RuntimeException e) {
                            Log.e(
                                    TAG,
                                    "Can't start "
                                        + "BraveMediaNotificationControllerServices.PlaybackListenerMicService", // presubmit: ignore-long-line
                                    e);
                        }
                    }
                };
    }
}
