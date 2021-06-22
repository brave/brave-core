/**
 * Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.vpn;

import android.content.Context;
import android.content.Intent;
import android.net.ConnectivityManager;
import android.net.Ikev2VpnProfile;
import android.net.NetworkCapabilities;
import android.net.NetworkInfo;
import android.os.Build;

import org.chromium.chrome.browser.vpn.BraveVpnPlansActivity;

public class BraveVpnUtils {
    public static boolean isVPNConnected(Context context) {
        ConnectivityManager connectivityManager = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
        if (connectivityManager != null) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                NetworkCapabilities capabilities = connectivityManager.getNetworkCapabilities(connectivityManager.getActiveNetwork());
                return capabilities.hasTransport(NetworkCapabilities.TRANSPORT_VPN);
            } else {
                NetworkInfo activeNetwork = connectivityManager.getActiveNetworkInfo();
                return activeNetwork.getType() == ConnectivityManager.TYPE_VPN;
            }
        }
        return false;
    }

    public static Ikev2VpnProfile getVpnProfile(Context context) {
        Ikev2VpnProfile.Builder builder = new Ikev2VpnProfile.Builder("Server Url", "identity");
        return builder.setAuthUsernamePassword("username", "password", null).build();
    }

    public static boolean isBraveVpnFeatureEnable() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            return true;
        }
        return false;
    }

    public static void openBraveVpnPlansActivity(Context context) {
        Intent braveVpnPlanIntent = new Intent(context, BraveVpnPlansActivity.class);
        braveVpnPlanIntent.setFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP);
        context.startActivity(braveVpnPlanIntent);
    }
}
