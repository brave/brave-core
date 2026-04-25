/**
 * Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.vpn;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

import org.chromium.chrome.browser.vpn.wireguard.WireguardService;

public class DisconnectVpnBroadcastReceiver extends BroadcastReceiver {
    public static String DISCONNECT_VPN_ACTION = "disconnect_vpn_action";

    @Override
    public void onReceive(Context context, Intent intent) {
        String action = intent.getAction();
        if (action != null && action.equals(DISCONNECT_VPN_ACTION)) {
            context.stopService(new Intent(context, WireguardService.class));
        }
    }
}
