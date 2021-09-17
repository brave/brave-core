/**
 * Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.vpn;

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
import org.chromium.chrome.browser.vpn.BraveVpnUtils;
import org.chromium.ui.widget.Toast;

public class BraveVpnProfileUtils {
    public static final int BRAVE_VPN_PROFILE_REQUEST_CODE = 36;

    private static BraveVpnProfileUtils sBraveVpnProfileUtils;
    private Context mContext;
    private VpnManager mVpnManager;

    public static BraveVpnProfileUtils getInstance(Context context) {
        if (sBraveVpnProfileUtils == null)
            sBraveVpnProfileUtils = new BraveVpnProfileUtils(context);
        return sBraveVpnProfileUtils;
    }

    public VpnManager getVpnManager() {
        if (mVpnManager == null) {
            mVpnManager = (VpnManager) mContext.getSystemService(Context.VPN_MANAGEMENT_SERVICE);
        }
        return mVpnManager;
    }

    private BraveVpnProfileUtils(Context context) {
        this.mContext = context;
        if (mVpnManager == null) {
            mVpnManager = (VpnManager) context.getSystemService(Context.VPN_MANAGEMENT_SERVICE);
        }
    }

    public boolean isVPNConnected() {
        ConnectivityManager connectivityManager =
                (ConnectivityManager) mContext.getSystemService(Context.CONNECTIVITY_SERVICE);
        if (connectivityManager != null) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                NetworkCapabilities capabilities = connectivityManager.getNetworkCapabilities(
                        connectivityManager.getActiveNetwork());
                return capabilities.hasTransport(NetworkCapabilities.TRANSPORT_VPN);
            } else {
                NetworkInfo activeNetwork = connectivityManager.getActiveNetworkInfo();
                return activeNetwork.getType() == ConnectivityManager.TYPE_VPN;
            }
        }
        return false;
    }

    public Ikev2VpnProfile getVpnProfile(String hostname, String username, String password) {
        Ikev2VpnProfile.Builder builder = new Ikev2VpnProfile.Builder(hostname, hostname);
        return builder.setAuthUsernamePassword(username, password, null).build();
    }

    public void startStopVpn() {
        if (!isVPNConnected()) {
            try {
                startVpn();
            } catch (SecurityException securityException) {
                Toast.makeText(mContext, R.string.vpn_profile_is_not_created, Toast.LENGTH_SHORT)
                        .show();
                BraveVpnUtils.dismissProgressDialog();
                BraveVpnUtils.openBraveVpnProfileActivity(mContext);
            }
        } else {
            stopVpn();
        }
    }

    public void startVpn() {
        getVpnManager().startProvisionedVpnProfile();
    }

    public void stopVpn() {
        getVpnManager().stopProvisionedVpnProfile();
    }

    public void deleteVpnProfile() {
        getVpnManager().deleteProvisionedVpnProfile();
    }

    public void createVpnProfile(
            Activity activity, String hostname, String username, String password) {
        Ikev2VpnProfile ikev2VpnProfile = getVpnProfile(hostname, username, password);
        Intent intent = getVpnManager().provisionVpnProfile(ikev2VpnProfile);
        activity.startActivityForResult(intent, BRAVE_VPN_PROFILE_REQUEST_CODE);
    }
}
