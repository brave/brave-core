/*
  Copyright (c) 2022 The Brave Authors. All rights reserved.
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this file,
  You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.widget.quickactionsearchandbookmark.utils;

import android.app.PendingIntent;
import android.appwidget.AppWidgetManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.os.Build;
import android.os.Bundle;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.browser.preferences.SharedPreferencesManager;
import org.chromium.chrome.browser.widget.quickactionsearchandbookmark.QuickActionSearchAndBookmarkWidgetProvider;

public class BraveSearchWidgetUtils {
    private static final String SHOW_WIDGET =
            "org.chromium.chrome.browser.widget.quickactionsearchandbookmark.utils.SHOW_WIDGET";

    public static boolean getShouldShowWidgetPromo() {
        return SharedPreferencesManager.getInstance().readBoolean(
                SHOW_WIDGET, isRequestPinAppWidgetSupported());
    }

    public static void setShouldShowWidgetPromo(boolean shouldShow) {
        SharedPreferencesManager.getInstance().writeBoolean(SHOW_WIDGET, shouldShow);
    }

    public static boolean isRequestPinAppWidgetSupported() {
        AppWidgetManager appWidgetManager =
                ContextUtils.getApplicationContext().getSystemService(AppWidgetManager.class);
        return Build.VERSION.SDK_INT >= Build.VERSION_CODES.O && appWidgetManager != null
                && appWidgetManager.isRequestPinAppWidgetSupported();
    }

    public static void requestPinAppWidget() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            Context context = ContextUtils.getApplicationContext();
            AppWidgetManager appWidgetManager = context.getSystemService(AppWidgetManager.class);

            ComponentName appWidgetProvider =
                    new ComponentName(context, QuickActionSearchAndBookmarkWidgetProvider.class);

            if (appWidgetManager != null && appWidgetManager.isRequestPinAppWidgetSupported()) {
                Bundle bundle = new Bundle();
                bundle.putBoolean(QuickActionSearchAndBookmarkWidgetProvider.FROM_SETTINGS, true);
                Intent pinnedWidgetCallbackIntent =
                        new Intent(context, QuickActionSearchAndBookmarkWidgetProvider.class);
                pinnedWidgetCallbackIntent.putExtras(bundle);
                PendingIntent successCallback =
                        PendingIntent.getBroadcast(context, 0, pinnedWidgetCallbackIntent,
                                PendingIntent.FLAG_IMMUTABLE | PendingIntent.FLAG_UPDATE_CURRENT);

                appWidgetManager.requestPinAppWidget(appWidgetProvider, null, successCallback);
            }
        }
    }
}
