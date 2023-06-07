/**
 * Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.vpn.utils;

import android.content.Context;
import android.content.Intent;
import android.net.ConnectivityManager;
import android.net.NetworkCapabilities;
import android.net.NetworkInfo;
import android.os.Build;

import androidx.core.content.ContextCompat;

import org.chromium.chrome.browser.vpn.wireguard.WireguardService;
import org.chromium.chrome.browser.vpn.wireguard.WireguardUtils;

public class BraveVpnProfileUtils {
    private static volatile BraveVpnProfileUtils sBraveVpnProfileUtils;
    private static Object mutex = new Object();

    private BraveVpnProfileUtils() {}

    public static BraveVpnProfileUtils getInstance() {
        BraveVpnProfileUtils result = sBraveVpnProfileUtils;
        if (result == null) {
            synchronized (mutex) {
                result = sBraveVpnProfileUtils;
                if (result == null) sBraveVpnProfileUtils = result = new BraveVpnProfileUtils();
            }
        }
        return result;
    }

    public boolean isBraveVPNConnected(Context context) {
        return WireguardUtils.isServiceRunningInForeground(context, WireguardService.class);
    }

    public boolean isVPNRunning(Context context) {
        ConnectivityManager connectivityManager =
                (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
        boolean isVpnConnected = false;
        if (connectivityManager != null) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                NetworkCapabilities capabilities = connectivityManager.getNetworkCapabilities(
                        connectivityManager.getActiveNetwork());
                isVpnConnected = capabilities != null
                        ? capabilities.hasTransport(NetworkCapabilities.TRANSPORT_VPN)
                        : false;
            } else {
                NetworkInfo activeNetwork = connectivityManager.getActiveNetworkInfo();
                isVpnConnected = activeNetwork.getType() == ConnectivityManager.TYPE_VPN;
            }
        }
        return isVpnConnected;
    }

    public void startVpn(Context context) {
        ContextCompat.startForegroundService(context, new Intent(context, WireguardService.class));
    }

    public void stopVpn(Context context) {
        context.stopService(new Intent(context, WireguardService.class));
    }
}
