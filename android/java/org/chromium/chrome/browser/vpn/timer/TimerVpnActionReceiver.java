/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.vpn.timer;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

import org.chromium.chrome.browser.vpn.utils.BraveVpnProfileUtils;

public class TimerVpnActionReceiver extends BroadcastReceiver {
    @Override
    public void onReceive(Context context, Intent intent) {
        try {
            BraveVpnProfileUtils.getInstance().startVpn(context);
        } catch (Exception exc) {
            // There could be uninitialized parts on early stages. Just ignore it the exception,
            // it's better comparing to crashing
        }
    }
}
