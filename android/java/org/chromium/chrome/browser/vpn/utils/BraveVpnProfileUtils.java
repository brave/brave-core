/**
 * Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.vpn.utils;

import android.annotation.TargetApi;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.net.ConnectivityManager;
import android.net.Ikev2VpnProfile;
import android.net.NetworkCapabilities;
import android.net.NetworkInfo;
import android.net.VpnManager;
import android.os.Build;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.vpn.activities.BraveVpnProfileActivity;
import org.chromium.chrome.browser.vpn.utils.BraveVpnPrefUtils;
import org.chromium.chrome.browser.vpn.utils.BraveVpnUtils;
import org.chromium.ui.widget.Toast;

@TargetApi(Build.VERSION_CODES.R)
public class BraveVpnProfileUtils {
    public static final int BRAVE_VPN_PROFILE_REQUEST_CODE = 36;

    private static BraveVpnProfileUtils sBraveVpnProfileUtils;
    private VpnManager mVpnManager;

    public static BraveVpnProfileUtils getInstance() {
        if (sBraveVpnProfileUtils == null) sBraveVpnProfileUtils = new BraveVpnProfileUtils();
        return sBraveVpnProfileUtils;
    }

    public VpnManager getVpnManager(Context context) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            if (mVpnManager == null) {
                mVpnManager = (VpnManager) context.getSystemService(Context.VPN_MANAGEMENT_SERVICE);
            }
            return mVpnManager;
        }
        return null;
    }

    public boolean isVPNConnected(Context context) {
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

    private Ikev2VpnProfile getVpnProfile(String hostname, String username, String password) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            Ikev2VpnProfile.Builder builder =
                    new Ikev2VpnProfile.Builder(hostname, hostname).setMetered(false);
            return builder.setAuthUsernamePassword(username, password, null).build();
        }
        return null;
    }

    public void startStopVpn(Context context) {
        if (!isVPNConnected(context)) {
            Log.e("BraveVPN", "startStopVpn : 1");
            try {
                Log.e("BraveVPN", "startStopVpn : 2");
                startVpn(context);
            } catch (SecurityException securityException) {
                Log.e("BraveVPN", "startStopVpn : 3");
                Toast.makeText(context, R.string.vpn_profile_is_not_created, Toast.LENGTH_SHORT)
                        .show();
                BraveVpnUtils.dismissProgressDialog();
                BraveVpnUtils.openBraveVpnProfileActivity(context);
            }
        } else {
            Log.e("BraveVPN", "startStopVpn : 4");
            stopVpn(context);
        }
    }

    public void startVpn(Context context) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R && getVpnManager(context) != null) {
            getVpnManager(context).startProvisionedVpnProfile();
            BraveVpnPrefUtils.setVpnStart(true);
        }
    }

    public void stopVpn(Context context) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R && getVpnManager(context) != null) {
            getVpnManager(context).stopProvisionedVpnProfile();
        }
    }

    public void deleteVpnProfile(Context context) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R && getVpnManager(context) != null) {
            try {
                getVpnManager(context).deleteProvisionedVpnProfile();
            } catch (SecurityException securityException) {
                Log.e("BraveVPN", securityException.getMessage());
            }
        }
    }

    public void createVpnProfile(
            Activity activity, String hostname, String username, String password) {
        Log.e("BraveVPN", "createVpnProfile : 1");
        Ikev2VpnProfile ikev2VpnProfile = getVpnProfile(hostname, username, password);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R && ikev2VpnProfile != null) {
            Intent intent = getVpnManager(activity).provisionVpnProfile(ikev2VpnProfile);
            Log.e("BraveVPN", "createVpnProfile : 2");
            if (intent != null) {
                Log.e("BraveVPN", "createVpnProfile : 3");
                activity.startActivityForResult(intent, BRAVE_VPN_PROFILE_REQUEST_CODE);
            } else {
                Log.e("BraveVPN", "createVpnProfile : 4");
                startVpn(activity);
                BraveVpnUtils.showVpnConfirmDialog(activity);
            }
        }
    }
}
