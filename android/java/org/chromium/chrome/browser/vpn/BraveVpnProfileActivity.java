/**
 * Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.vpn;

import android.graphics.Paint;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.TextView;
import android.net.VpnManager;
import android.content.Intent;
import android.content.Context;
import android.net.Ikev2VpnProfile;
import android.view.View;
import android.widget.Button;

import androidx.appcompat.app.ActionBar;
import androidx.appcompat.widget.Toolbar;
import androidx.viewpager.widget.ViewPager;

import com.google.android.material.tabs.TabLayout;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.firstrun.FirstRunFlowSequencer;
import org.chromium.chrome.browser.init.AsyncInitializationActivity;
import org.chromium.chrome.browser.vpn.BraveVpnPlanPagerAdapter;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.vpn.BraveVpnConfirmDialogFragment;

public class BraveVpnProfileActivity extends AsyncInitializationActivity {
    private FirstRunFlowSequencer mFirstRunFlowSequencer;
    private VpnManager vpnManager;

    private void initializeViews() {
        setContentView(R.layout.activity_brave_vpn_profile);

        vpnManager = (VpnManager) getSystemService(Context.VPN_MANAGEMENT_SERVICE);

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        ActionBar actionBar = getSupportActionBar();
        assert actionBar != null;
        actionBar.setDisplayHomeAsUpEnabled(true);
        actionBar.setHomeAsUpIndicator(R.drawable.ic_baseline_close_24);
        actionBar.setTitle(getResources().getString(R.string.install_vpn));

        Button installVpnButton = findViewById(R.id.btn_install_profile);
        installVpnButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                createVpnProfileAndStartConnection();
            }
        });

        Button contactSupportButton = findViewById(R.id.btn_contact_supoort);
        contactSupportButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                
            }
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

    private void createVpnProfileAndStartConnection() {
        if (vpnManager != null) {
            Ikev2VpnProfile ikev2VpnProfile =
                            BraveVpnUtils.getVpnProfile(BraveVpnProfileActivity.this);
                    Intent intent = vpnManager.provisionVpnProfile(ikev2VpnProfile);
                    startActivityForResult(intent, BraveActivity.BRAVE_VPN_PROFILE_REQUEST_CODE);
        }
    }

    @Override
    public void onActivityResult (int requestCode, int resultCode,
                                  Intent data) {
        if (resultCode == RESULT_OK && requestCode == BraveActivity.BRAVE_VPN_PROFILE_REQUEST_CODE
                && BraveVpnUtils.isBraveVpnFeatureEnable()) {
            vpnManager.startProvisionedVpnProfile();
        BraveVpnConfirmDialogFragment braveVpnConfirmDialogFragment = new BraveVpnConfirmDialogFragment();
        braveVpnConfirmDialogFragment.setCancelable(false);
        braveVpnConfirmDialogFragment.show(getSupportFragmentManager(), "BraveVpnConfirmDialogFragment");
        }
        super.onActivityResult(requestCode, resultCode, data);
    }
}
