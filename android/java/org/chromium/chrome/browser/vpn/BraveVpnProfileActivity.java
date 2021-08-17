/**
 * Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.vpn;

import static com.android.billingclient.api.BillingClient.SkuType.SUBS;

import android.content.Context;
import android.content.Intent;
import android.graphics.Paint;
import android.net.VpnManager;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import androidx.appcompat.app.ActionBar;
import androidx.appcompat.widget.Toolbar;
import androidx.viewpager.widget.ViewPager;

import com.android.billingclient.api.BillingClient;
import com.android.billingclient.api.BillingClientStateListener;
import com.android.billingclient.api.BillingResult;
import com.android.billingclient.api.Purchase;
import com.android.billingclient.api.PurchasesUpdatedListener;
import com.google.android.material.tabs.TabLayout;

import org.json.JSONException;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.firstrun.FirstRunFlowSequencer;
import org.chromium.chrome.browser.init.AsyncInitializationActivity;
import org.chromium.chrome.browser.vpn.BraveVpnConfirmDialogFragment;
import org.chromium.chrome.browser.vpn.BraveVpnNativeWorker;
import org.chromium.chrome.browser.vpn.BraveVpnObserver;
import org.chromium.chrome.browser.vpn.BraveVpnPlanPagerAdapter;
import org.chromium.chrome.browser.vpn.BraveVpnUtils;
import org.chromium.chrome.browser.vpn.VpnProfileUtils;
import org.chromium.ui.widget.Toast;

import java.util.List;
import java.util.TimeZone;

public class BraveVpnProfileActivity extends BraveVpnParentActivity {
    private FirstRunFlowSequencer mFirstRunFlowSequencer;
    private TextView profileTitle;
    private TextView profileText;
    private Button installVpnButton;
    private Button contactSupportButton;

    @Override
    public void onResumeWithNative() {
        super.onResumeWithNative();
        BraveVpnNativeWorker.getInstance().addObserver(this);
    }

    @Override
    public void onPauseWithNative() {
        BraveVpnNativeWorker.getInstance().removeObserver(this);
        super.onPauseWithNative();
    }

    private void initializeViews() {
        setContentView(R.layout.activity_brave_vpn_profile);

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        ActionBar actionBar = getSupportActionBar();
        assert actionBar != null;
        actionBar.setDisplayHomeAsUpEnabled(true);
        actionBar.setHomeAsUpIndicator(R.drawable.ic_baseline_close_24);
        actionBar.setTitle(getResources().getString(R.string.install_vpn));

        profileTitle = findViewById(R.id.brave_vpn_profile_title);
        profileText = findViewById(R.id.brave_vpn_profile_text);

        installVpnButton = findViewById(R.id.btn_install_profile);
        installVpnButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                verifySubscription(false);
            }
        });

        contactSupportButton = findViewById(R.id.btn_contact_supoort);
        contactSupportButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {}
        });
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item.getItemId() == android.R.id.home) {
            finish();
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    protected void triggerLayoutInflation() {
        mFirstRunFlowSequencer = new FirstRunFlowSequencer(this) {
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
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (resultCode == RESULT_OK && requestCode == VpnProfileUtils.BRAVE_VPN_PROFILE_REQUEST_CODE
                && BraveVpnUtils.isBraveVpnFeatureEnable()) {
            VpnManager vpnManager = (VpnManager) getSystemService(Context.VPN_MANAGEMENT_SERVICE);
            if (vpnManager != null) {
                vpnManager.startProvisionedVpnProfile();
                BraveVpnConfirmDialogFragment braveVpnConfirmDialogFragment =
                        new BraveVpnConfirmDialogFragment();
                braveVpnConfirmDialogFragment.setCancelable(false);
                braveVpnConfirmDialogFragment.show(
                        getSupportFragmentManager(), "BraveVpnConfirmDialogFragment");
            }
        } else if (resultCode == RESULT_CANCELED) {
            profileTitle.setText(getResources().getString(R.string.some_context));
            profileText.setText(getResources().getString(R.string.some_context_text));
            installVpnButton.setText(getResources().getString(R.string.accept_connection_request));
            contactSupportButton.setVisibility(View.GONE);
        }
        super.onActivityResult(requestCode, resultCode, data);
    }
}
