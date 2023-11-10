/**
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.vpn.timer;

import android.app.AlarmManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;

import org.chromium.base.IntentUtils;
import org.chromium.chrome.browser.vpn.timer.TimerDialogFragment;
import org.chromium.chrome.browser.vpn.timer.TimerVpnActionReceiver;

import java.util.Calendar;
import java.util.Date;

public class TimerUtils {
    private static int VPN_ACTION_REQUEST_CODE = 1001;

    public static void scheduleVpnAction(Context context, int minutes) {
        Intent vpnActionIntent = new Intent(context, TimerVpnActionReceiver.class);
        PendingIntent pendingIntent = PendingIntent.getBroadcast(context, VPN_ACTION_REQUEST_CODE,
                vpnActionIntent, 0 | IntentUtils.getPendingIntentMutabilityFlag(true));
        Calendar calendar = Calendar.getInstance();
        calendar.setTimeInMillis(System.currentTimeMillis());
        calendar.add(Calendar.MINUTE, minutes);

        Date date = calendar.getTime();
        AlarmManager alarmManager = (AlarmManager) context.getSystemService(Context.ALARM_SERVICE);
        assert alarmManager != null;
        alarmManager.set(AlarmManager.RTC_WAKEUP, date.getTime(), pendingIntent);
    }

    public static void cancelScheduledVpnAction(Context context) {
        Intent vpnActionIntent = new Intent(context, TimerVpnActionReceiver.class);
        PendingIntent pendingIntent = PendingIntent.getBroadcast(context, VPN_ACTION_REQUEST_CODE,
                vpnActionIntent, 0 | IntentUtils.getPendingIntentMutabilityFlag(true));

        AlarmManager alarmManager = (AlarmManager) context.getSystemService(Context.ALARM_SERVICE);
        assert alarmManager != null;
        alarmManager.cancel(pendingIntent);
    }
}
