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
import android.net.NetworkInfo;
import android.net.Uri;
import android.os.Bundle;
import android.telephony.TelephonyManager;
import android.view.View;
import android.widget.Button;

import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.SwitchCompat;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.about_settings.AboutChromeSettings;
import org.chromium.chrome.browser.about_settings.AboutSettingsBridge;
import org.chromium.chrome.browser.vpn.BraveVpnUtils;

import java.util.TimeZone;

public class BraveVpnSupportActivity extends AppCompatActivity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_brave_vpn_support);

        SwitchCompat vpnHostnameSwitch = findViewById(R.id.vpn_hostname_switch);
        SwitchCompat subscriptionTypeSwitch = findViewById(R.id.subscription_type_switch);
        SwitchCompat appVersionSwitch = findViewById(R.id.app_version_switch);
        SwitchCompat timezoneSwitch = findViewById(R.id.timezone_switch);
        SwitchCompat networkTypeSwitch = findViewById(R.id.network_type_switch);
        SwitchCompat cellularCarrierSwitch = findViewById(R.id.cellular_carrier_switch);

        Button btnContinueToEmail = findViewById(R.id.btn_continue_to_email);
        btnContinueToEmail.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                StringBuilder bodyText =
                        new StringBuilder("#### Please do not edit any information below ####\n");
                if (vpnHostnameSwitch.isChecked()) {
                    bodyText.append("\n\nVPN Hostname : " + BraveVpnUtils.getHostname());
                }
                if (subscriptionTypeSwitch.isChecked()) {
                    bodyText.append("\n\nSubscription Type : Yearly Subscription");
                }
                if (appVersionSwitch.isChecked()) {
                    bodyText.append("\n\nApp Version : "
                            + AboutChromeSettings.getApplicationVersion(
                                    BraveVpnSupportActivity.this,
                                    AboutSettingsBridge.getApplicationVersion()));
                }
                if (timezoneSwitch.isChecked()) {
                    bodyText.append("\n\nTimezone : ").append(TimeZone.getDefault().getID());
                }
                if (networkTypeSwitch.isChecked()) {
                    bodyText.append("\n\nNetwork Type : ").append(getNetworkType());
                }
                if (cellularCarrierSwitch.isChecked()) {
                    bodyText.append("\n\nCellular Carrier : ").append(getCellularCarrier());
                }
                composeEmail(bodyText.toString());
            }
        });
    }

    public void composeEmail(String body) {
        Intent intent = new Intent(Intent.ACTION_SENDTO);
        intent.setData(Uri.parse("mailto:")); // only email apps should handle this
        intent.putExtra(Intent.EXTRA_EMAIL, new String[] {"brave@guardianapp.com"});
        intent.putExtra(Intent.EXTRA_SUBJECT, "Brave Firewall + VPN Issue");
        intent.putExtra(Intent.EXTRA_TEXT, body);
        finish();
        startActivity(intent);
    }

    public String getNetworkType() {
        ConnectivityManager cm =
                (ConnectivityManager) getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkInfo activeNetwork = cm.getActiveNetworkInfo();
        if (activeNetwork != null) { // connected to the internet
            if (activeNetwork.getType() == ConnectivityManager.TYPE_WIFI) {
                // connected to wifi
                return activeNetwork.getTypeName();
            } else if (activeNetwork.getType() == ConnectivityManager.TYPE_MOBILE) {
                // connected to the mobile provider's data plan
                return activeNetwork.getTypeName();
            }
        }
        return "";
    }

    private String getCellularCarrier() {
        TelephonyManager telephonyManager =
                ((TelephonyManager) getSystemService(Context.TELEPHONY_SERVICE));
        return telephonyManager.getNetworkOperatorName();
    }
}
