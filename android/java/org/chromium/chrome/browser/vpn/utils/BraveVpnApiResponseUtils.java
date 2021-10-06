/**
 * Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.vpn.utils;

import android.app.Activity;

import org.chromium.base.Log;
import org.chromium.chrome.browser.vpn.utils.BraveVpnPrefUtils;
import org.chromium.chrome.browser.vpn.utils.BraveVpnProfileUtils;
import org.chromium.chrome.browser.vpn.utils.BraveVpnUtils;
import org.chromium.ui.widget.Toast;

import java.util.List;

public class BraveVpnApiResponseUtils {
    public static void queryPurchaseFailed(Activity activity) {
        Log.e("BraveVPN", "BraveVpnApiResponseUtils 4");
        BraveVpnPrefUtils.setPurchaseToken("");
        BraveVpnPrefUtils.setProductId("");
        BraveVpnPrefUtils.setPurchaseExpiry(0L);
        BraveVpnPrefUtils.setSubscriptionPurchase(false);
        if (BraveVpnProfileUtils.getInstance().isVPNConnected(activity)) {
            BraveVpnProfileUtils.getInstance().stopVpn(activity);
        }
        BraveVpnProfileUtils.getInstance().deleteVpnProfile(activity);
        Toast.makeText(activity, R.string.purchase_token_verification_failed, Toast.LENGTH_LONG)
                .show();
        BraveVpnUtils.dismissProgressDialog();
        BraveVpnUtils.openBraveVpnPlansActivity(activity);
    }
}
