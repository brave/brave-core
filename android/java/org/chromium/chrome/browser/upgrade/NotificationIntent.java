/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.upgrade;

import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.Uri;
import android.os.Build;

import androidx.core.app.NotificationCompat;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.IntentUtils;
import org.chromium.chrome.R;

import java.util.Arrays;
import java.util.List;
import java.util.Locale;

/**
 * Fires notification about new features.
 */
public class NotificationIntent {
    private static final String TAG = "NotificationIntent";
    private static final String NOTIFICATION_TAG = "16c570a4-da7d-4c4e-8518-d2b7d6e41615";
    private static final String NOTIFICATION_CHANNEL_ID = "a79c3102-4183-4001-a553-ec3041bd0f49";
    private static final String URL = "https://support.brave.com/hc/en-us/articles/360045401211/";
    private static final List<String> mWhitelistedRegionalLocales =
            Arrays.asList("en", "ru", "uk", "de", "pt", "pl", "ja", "es", "fr");
    private static final int NOTIFICATION_ID = 732;

    // private static final String NOTIFICATION_TITLE = "Brave update";
    // private static final String NOTIFICATION_TEXT = "The new Brave browser is 22% faster";

    public static void fireNotificationIfNecessary(Context context) {
        String notification_text = context.getString(R.string.update_notification_text);
        if (!shouldNotify(context)) {
            return;
        }
        NotificationManager mNotificationManager =
                (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            NotificationChannel channel =
                    new NotificationChannel(
                            NOTIFICATION_CHANNEL_ID,
                            "Channel for notifications",
                            NotificationManager.IMPORTANCE_DEFAULT);
            mNotificationManager.createNotificationChannel(channel);
        }

        NotificationCompat.Builder mBuilder =
            new NotificationCompat.Builder(context, NOTIFICATION_CHANNEL_ID);

        // Create an intent that’ll be fired when a user taps the notification//
        Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse(URL));
        intent.putExtra(BravePreferenceKeys.BRAVE_UPDATE_EXTRA_PARAM, true);
        intent.setPackage(context.getPackageName());
        PendingIntent pendingIntent = PendingIntent.getActivity(
                context, 0, intent, IntentUtils.getPendingIntentMutabilityFlag(true));

        mBuilder.setContentIntent(pendingIntent);
        mBuilder.setAutoCancel(true);

        mBuilder.setSmallIcon(R.drawable.ic_chrome);
        mBuilder.setContentTitle(context.getString(R.string.update_notification_title));
        mBuilder.setContentText(notification_text);
        mBuilder.setStyle(new NotificationCompat.BigTextStyle().bigText(notification_text));

        mNotificationManager.notify(NOTIFICATION_TAG, NOTIFICATION_ID, mBuilder.build());
    }

    private static boolean shouldNotify(Context context) {
        String deviceLanguage = Locale.getDefault().getLanguage();
        if (getPreferences(context) != 0
                || !mWhitelistedRegionalLocales.contains(
                        new Locale(deviceLanguage).getLanguage())) {
            return false;
        }

        return true;
    }

    private static long getPreferences(Context context) {
        SharedPreferences sharedPref =
                context.getSharedPreferences(BravePreferenceKeys.BRAVE_NOTIFICATION_PREF_NAME, 0);

        return sharedPref.getLong(BravePreferenceKeys.BRAVE_MILLISECONDS_NAME, 0);
    }
}
