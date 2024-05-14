/**
 * Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.vpn.activities;

import android.content.Context;
import android.content.Intent;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.Uri;
import android.os.Bundle;
import android.telephony.TelephonyManager;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;
import android.widget.RadioGroup;
import android.widget.TextView;

import androidx.appcompat.app.ActionBar;
import androidx.appcompat.widget.AppCompatRadioButton;
import androidx.appcompat.widget.SwitchCompat;
import androidx.appcompat.widget.Toolbar;

import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.about_settings.AboutChromeSettings;
import org.chromium.chrome.browser.about_settings.AboutSettingsBridge;
import org.chromium.chrome.browser.firstrun.BraveFirstRunFlowSequencer;
import org.chromium.chrome.browser.init.ActivityProfileProvider;
import org.chromium.chrome.browser.init.AsyncInitializationActivity;
import org.chromium.chrome.browser.profiles.ProfileProvider;
import org.chromium.chrome.browser.vpn.utils.BraveVpnPrefUtils;

import java.util.TimeZone;

public class BraveVpnSupportActivity extends AsyncInitializationActivity {
    private BraveFirstRunFlowSequencer mFirstRunFlowSequencer;
    private final OneshotSupplier<ProfileProvider> mProfileSupplier;

    public BraveVpnSupportActivity() {
        mProfileSupplier = createProfileProvider();
    }

    private void initializeViews() {
        setContentView(R.layout.activity_brave_vpn_support);

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        ActionBar actionBar = getSupportActionBar();
        assert actionBar != null;
        actionBar.setDisplayHomeAsUpEnabled(true);
        actionBar.setTitle(getResources().getString(R.string.contact_technical_support));

        SwitchCompat vpnHostnameSwitch = findViewById(R.id.vpn_hostname_switch);
        SwitchCompat subscriptionTypeSwitch = findViewById(R.id.subscription_type_switch);
        SwitchCompat appReceiptSwitch = findViewById(R.id.app_receipt_switch);
        SwitchCompat appVersionSwitch = findViewById(R.id.app_version_switch);
        SwitchCompat timezoneSwitch = findViewById(R.id.timezone_switch);
        SwitchCompat networkTypeSwitch = findViewById(R.id.network_type_switch);
        SwitchCompat cellularCarrierSwitch = findViewById(R.id.cellular_carrier_switch);

        RadioGroup otherIssuesRadioGroup = findViewById(R.id.other_issues_radiogroup);

        TextView otherIssuesText = findViewById(R.id.other_issues_text);
        otherIssuesText.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (otherIssuesRadioGroup.isShown()) {
                    otherIssuesText.setCompoundDrawablesWithIntrinsicBounds(
                            0, 0, R.drawable.ic_toggle_down, 0);
                    otherIssuesRadioGroup.setVisibility(View.GONE);
                } else {
                    otherIssuesText.setCompoundDrawablesWithIntrinsicBounds(
                            0, 0, R.drawable.ic_toggle_up, 0);
                    otherIssuesRadioGroup.setVisibility(View.VISIBLE);
                }
            }
        });

        Button btnContinueToEmail = findViewById(R.id.btn_continue_to_email);
        otherIssuesRadioGroup.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(RadioGroup group, int checkedId) {
                if (!btnContinueToEmail.isEnabled()) {
                    btnContinueToEmail.setEnabled(true);
                    btnContinueToEmail.setBackgroundResource(R.drawable.orange_rounded_button);
                }
            }
        });

        btnContinueToEmail.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                StringBuilder bodyText =
                        new StringBuilder(getResources().getString(R.string.support_email_text));
                if (vpnHostnameSwitch.isChecked()) {
                    bodyText.append(String.format(getResources().getString(R.string.vpn_host_text),
                            BraveVpnPrefUtils.getHostname()));
                }
                if (subscriptionTypeSwitch.isChecked()) {
                    bodyText.append(
                            String.format(getResources().getString(R.string.subscription_type_text),
                                    BraveVpnPrefUtils.getProductId()));
                }
                if (appReceiptSwitch.isChecked()) {
                    bodyText.append(
                            String.format(getResources().getString(R.string.playstore_token_text),
                                    BraveVpnPrefUtils.getPurchaseToken()));
                }
                if (appVersionSwitch.isChecked()) {
                    bodyText.append(String.format(
                            getResources().getString(R.string.app_version_text),
                            AboutChromeSettings.getApplicationVersion(BraveVpnSupportActivity.this,
                                    AboutSettingsBridge.getApplicationVersion())));
                }
                if (timezoneSwitch.isChecked()) {
                    bodyText.append(getResources().getString(R.string.timezone_text))
                            .append(TimeZone.getDefault().getID());
                }
                if (networkTypeSwitch.isChecked()) {
                    bodyText.append(getResources().getString(R.string.network_type_text))
                            .append(getNetworkType());
                }
                if (cellularCarrierSwitch.isChecked()) {
                    bodyText.append(getResources().getString(R.string.cellular_carrier_text))
                            .append(getCellularCarrier());
                }

                bodyText.append(getResources().getString(R.string.other_issue_text));
                AppCompatRadioButton checkedRadioButton =
                        findViewById(otherIssuesRadioGroup.getCheckedRadioButtonId());
                bodyText.append(checkedRadioButton.getText()).append("\n");
                bodyText.append(getResources().getString(R.string.platform_text))
                        .append("Android")
                        .append("\n");

                composeEmail(bodyText.toString());
            }
        });
    }

    @Override
    protected void triggerLayoutInflation() {
        mFirstRunFlowSequencer =
                new BraveFirstRunFlowSequencer(mProfileSupplier) {
                    @Override
                    public void onFlowIsKnown(Bundle freProperties) {
                        initializeViews();
                    }
                };
        mFirstRunFlowSequencer.start();
        onInitialLayoutInflationComplete();
    }

    @Override
    public boolean shouldStartGpuProcess() {
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item.getItemId() == android.R.id.home) {
            finish();
        }
        return super.onOptionsItemSelected(item);
    }

    private void composeEmail(String body) {
        Intent intent = new Intent(Intent.ACTION_SENDTO);
        intent.setData(Uri.parse("mailto:")); // only email apps should handle this
        intent.putExtra(Intent.EXTRA_EMAIL, new String[] {"braveandroid@guardianapp.com"});
        intent.putExtra(Intent.EXTRA_SUBJECT, "Brave Firewall + VPN Issue");
        intent.putExtra(Intent.EXTRA_TEXT, body);
        finish();
        startActivity(intent);
    }

    private String getNetworkType() {
        ConnectivityManager cm =
                (ConnectivityManager) getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkInfo activeNetwork = cm.getActiveNetworkInfo();
        if (activeNetwork != null) {
            // connected to the internet
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

    @Override
    protected OneshotSupplier<ProfileProvider> createProfileProvider() {
        return new ActivityProfileProvider(getLifecycleDispatcher());
    }
}
