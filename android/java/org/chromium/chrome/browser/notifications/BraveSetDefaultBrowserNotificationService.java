/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.notifications;

import android.app.AlarmManager;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.provider.Settings;

import androidx.core.app.NotificationCompat;

import org.chromium.base.ContextUtils;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.components.embedder_support.util.UrlConstants;

import java.util.Calendar;

public class BraveSetDefaultBrowserNotificationService extends BroadcastReceiver {
    public Context mContext;
    private Intent mIntent;

    public static final String HAS_ASKED_AT_11_22 = "brave_set_default_browser_has_at_11_22";

    public static final int NOTIFICATION_ID = 10;

    // Deep links
    public static final String DEEP_LINK = "deep_link";
    public static final String SHOW_DEFAULT_APP_SETTINGS = "SHOW_DEFAULT_APP_SETTINGS";

    // Intent types
    public static final String INTENT_TYPE = "intent_type";
    public static final String AT_11_22 = "at_11_22";

    public static final String CANCEL_NOTIFICATION = "cancel_notification";

    // Startup notification data
    public static final String NOTIFICATION_ID_EXTRA = "notification_id_extra";

    private static final int NOTIFICATION_HOUR = 11;
    private static final int NOTIFICATION_MIN = 22;

    public static boolean isBraveSetAsDefaultBrowser(Context context) {
        Intent browserIntent =
                new Intent(Intent.ACTION_VIEW, Uri.parse(UrlConstants.HTTP_URL_PREFIX));
        ResolveInfo resolveInfo = context.getPackageManager().resolveActivity(
                browserIntent, supportsDefault() ? PackageManager.MATCH_DEFAULT_ONLY : 0);
        if (resolveInfo == null || resolveInfo.activityInfo == null || resolveInfo.activityInfo.packageName == null
                || ContextUtils.getApplicationContext() == null) {
            return false;
        }
        return resolveInfo.activityInfo.packageName.equals(
                ContextUtils.getApplicationContext().getPackageName());
    }

    private boolean shouldShowNotification() {
        if (isBraveSetAsDefaultBrowser(mContext) || mIntent.getStringExtra(INTENT_TYPE) == null) {
            return false;
        }

        if (mIntent.getStringExtra(INTENT_TYPE).equals(AT_11_22)) {
            return true;
        }

        // Shouldn't reach here. Just out of safety, don't annoy users
        return false;
    }

    private static boolean supportsDefault() {
        return Build.VERSION.SDK_INT >= Build.VERSION_CODES.N;
    }

    private boolean hasAlternateDefaultBrowser() {
        Intent browserIntent =
                new Intent(Intent.ACTION_VIEW, Uri.parse(UrlConstants.HTTP_URL_PREFIX));
        ResolveInfo resolveInfo = mContext.getPackageManager().resolveActivity(
                browserIntent, supportsDefault() ? PackageManager.MATCH_DEFAULT_ONLY : 0);
        return !(resolveInfo.activityInfo.packageName.equals(
                         BraveActivity.ANDROID_SETUPWIZARD_PACKAGE_NAME)
                || resolveInfo.activityInfo.packageName.equals(BraveActivity.ANDROID_PACKAGE_NAME));
    }

    private void showNotification() {
        NotificationCompat.Builder b =
                new NotificationCompat.Builder(mContext, BraveActivity.CHANNEL_ID);

        b.setSmallIcon(R.drawable.ic_chrome)
                .setAutoCancel(false)
                .setContentTitle(
                        "⚡" + mContext.getString(R.string.brave_default_browser_title) + "⚡")
                .setContentText(mContext.getString(R.string.brave_default_browser_body))
                .setStyle(new NotificationCompat.BigTextStyle().bigText(
                        mContext.getString(R.string.brave_default_browser_body)))
                .setPriority(NotificationCompat.PRIORITY_DEFAULT)
                .setCategory(NotificationCompat.CATEGORY_MESSAGE);

        if (supportsDefault()) {
            PendingIntent intentYes = getDefaultAppSettingsIntent(mContext);
            NotificationCompat.Action actionYes =
                    new NotificationCompat.Action
                            .Builder(0, mContext.getString(R.string.brave_make_default_text),
                                    intentYes)
                            .build();
            NotificationCompat.Action actionNo =
                    new NotificationCompat.Action
                            .Builder(0, mContext.getString(R.string.brave_make_default_no),
                                    getDismissIntent(mContext, NOTIFICATION_ID))
                            .build();
            b.addAction(actionNo);
            b.addAction(actionYes);
            b.setContentIntent(intentYes);
        }

        NotificationManager notificationManager =
                (NotificationManager) mContext.getSystemService(Context.NOTIFICATION_SERVICE);
        notificationManager.notify(NOTIFICATION_ID, b.build());
    }

    public static PendingIntent getDefaultAppSettingsIntent(Context context) {
        Intent intent = new Intent(context, BraveSetDefaultBrowserNotificationService.class);
        intent.setAction(DEEP_LINK);
        intent.putExtra(DEEP_LINK, SHOW_DEFAULT_APP_SETTINGS);
        intent.putExtra(NOTIFICATION_ID_EXTRA, NOTIFICATION_ID);
        return PendingIntent.getBroadcast(context, 0, intent, PendingIntent.FLAG_UPDATE_CURRENT);
    }

    public static PendingIntent getDismissIntent(Context context, int notification_id) {
        Intent intent = new Intent(context, BraveSetDefaultBrowserNotificationService.class);
        intent.setAction(CANCEL_NOTIFICATION);
        intent.putExtra(NOTIFICATION_ID_EXTRA, notification_id);

        return PendingIntent.getBroadcast(context, notification_id, intent, 0);
    }

    private boolean hasAskedAt1122() {
        SharedPreferences sharedPref = ContextUtils.getAppSharedPreferences();
        // Check to see if we already asked at 11:22
        return sharedPref.getBoolean(HAS_ASKED_AT_11_22, false);
    }

    private void setAlarmFor1122() {
        SharedPreferences sharedPref = ContextUtils.getAppSharedPreferences();
        SharedPreferences.Editor editor = sharedPref.edit();
        editor.putBoolean(HAS_ASKED_AT_11_22, true);
        editor.apply();

        Calendar currentTime = Calendar.getInstance();
        if (currentTime.get(Calendar.HOUR_OF_DAY) > NOTIFICATION_HOUR
                || (currentTime.get(Calendar.HOUR_OF_DAY) == NOTIFICATION_HOUR
                        && currentTime.get(Calendar.MINUTE) >= NOTIFICATION_MIN)) {
            // Current time is after 11:22, so set alarm on tomorrow
            currentTime.add(Calendar.DAY_OF_YEAR, 1);
        }
        currentTime.set(Calendar.HOUR_OF_DAY, NOTIFICATION_HOUR);
        currentTime.set(Calendar.MINUTE, NOTIFICATION_MIN);
        currentTime.set(Calendar.SECOND, 0);
        currentTime.set(Calendar.MILLISECOND, 0);
        AlarmManager am = (AlarmManager) mContext.getSystemService(Context.ALARM_SERVICE);
        Intent intent = new Intent(mContext, BraveSetDefaultBrowserNotificationService.class);
        intent.putExtra(INTENT_TYPE, AT_11_22);
        intent.setAction(AT_11_22);
        am.setExact(AlarmManager.RTC_WAKEUP, currentTime.getTimeInMillis(),
                PendingIntent.getBroadcast(mContext, 0, intent, PendingIntent.FLAG_UPDATE_CURRENT));
    }

    private void handleBraveSetDefaultBrowserDeepLink(Intent intent) {
        Bundle bundle = intent.getExtras();
        if (bundle.getString(BraveSetDefaultBrowserNotificationService.DEEP_LINK)
                        .equals(BraveSetDefaultBrowserNotificationService
                                        .SHOW_DEFAULT_APP_SETTINGS)) {
            Intent settingsIntent = hasAlternateDefaultBrowser()
                    ? new Intent(Settings.ACTION_MANAGE_DEFAULT_APPS_SETTINGS)
                    : new Intent(Intent.ACTION_VIEW, Uri.parse(BraveActivity.BRAVE_BLOG_URL));
            settingsIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            mContext.startActivity(settingsIntent);
        }
    }

    private class OnReceiveRunnable implements Runnable {
        @Override
        public void run() {
          boolean deepLinkIsHandled = false;
          if (mIntent != null
                  && mIntent.hasExtra(BraveSetDefaultBrowserNotificationService.DEEP_LINK)) {
              handleBraveSetDefaultBrowserDeepLink(mIntent);
              deepLinkIsHandled = true;
          }
          if (deepLinkIsHandled
                  || (mIntent != null && mIntent.getAction() != null
                          && mIntent.getAction().equals(CANCEL_NOTIFICATION))) {
              int notification_id = mIntent.getIntExtra(NOTIFICATION_ID_EXTRA, 0);
              NotificationManager notificationManager =
                      (NotificationManager) mContext.getSystemService(Context.NOTIFICATION_SERVICE);
              notificationManager.cancel(notification_id);
              return;
          }

          if (shouldShowNotification()) {
              showNotification();
          } else if (!hasAskedAt1122()) {
              setAlarmFor1122();
          }
        }
    }

    @Override
    public void onReceive(Context context, Intent intent) {
        mContext = context;
        mIntent = intent;
        // Work is done in IO thread because
        // ApplicationPackageManager.resolveActivity may cause file IO operation
        PostTask.postTask(TaskTraits.BEST_EFFORT_MAY_BLOCK, new OnReceiveRunnable());
    }
}
