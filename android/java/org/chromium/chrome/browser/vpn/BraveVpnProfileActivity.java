/**
 * Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.vpn;

import android.content.Intent;
import android.os.Bundle;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.TextView;

import androidx.appcompat.app.ActionBar;
import androidx.appcompat.widget.Toolbar;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.firstrun.FirstRunFlowSequencer;
import org.chromium.chrome.browser.vpn.BraveVpnNativeWorker;
import org.chromium.chrome.browser.vpn.BraveVpnProfileUtils;
import org.chromium.chrome.browser.vpn.BraveVpnUtils;

public class BraveVpnProfileActivity extends BraveVpnParentActivity {
    private FirstRunFlowSequencer mFirstRunFlowSequencer;
    private TextView mProfileTitle;
    private TextView mProfileText;
    private Button mInstallVpnButton;
    private Button mContactSupportButton;
    private ProgressBar mProfileProgress;
    private LinearLayout mProfileLayout;

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

        mProfileProgress = findViewById(R.id.profile_progress);
        mProfileLayout = findViewById(R.id.profile_layout);

        mProfileTitle = findViewById(R.id.brave_vpn_profile_title);
        mProfileText = findViewById(R.id.brave_vpn_profile_text);

        mInstallVpnButton = findViewById(R.id.btn_install_profile);
        mInstallVpnButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                verifySubscription();
            }
        });

        mContactSupportButton = findViewById(R.id.btn_contact_supoort);
        mContactSupportButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                BraveVpnUtils.openBraveVpnSupportActivity(BraveVpnProfileActivity.this);
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

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (resultCode == RESULT_OK
                && requestCode == BraveVpnProfileUtils.BRAVE_VPN_PROFILE_REQUEST_CODE
                && BraveVpnUtils.isBraveVpnFeatureEnable()) {
            BraveVpnProfileUtils.getInstance(BraveVpnProfileActivity.this).startVpn();
            BraveVpnConfirmDialogFragment braveVpnConfirmDialogFragment =
                    new BraveVpnConfirmDialogFragment();
            braveVpnConfirmDialogFragment.setCancelable(false);
            braveVpnConfirmDialogFragment.show(
                    getSupportFragmentManager(), "BraveVpnConfirmDialogFragment");
        } else if (resultCode == RESULT_CANCELED) {
            mProfileTitle.setText(getResources().getString(R.string.some_context));
            mProfileText.setText(getResources().getString(R.string.some_context_text));
            mInstallVpnButton.setText(getResources().getString(R.string.accept_connection_request));
            mContactSupportButton.setVisibility(View.GONE);
        }
        hideProgress();
        super.onActivityResult(requestCode, resultCode, data);
    }

    @Override
    public void showRestoreMenu(boolean shouldShowRestore) {}

    @Override
    public void showProgress() {
        if (mProfileProgress != null) {
            mProfileProgress.setVisibility(View.VISIBLE);
        }
        if (mProfileLayout != null) {
            mProfileLayout.setAlpha(0.4f);
        }
    }

    @Override
    public void hideProgress() {
        if (mProfileProgress != null) {
            mProfileProgress.setVisibility(View.GONE);
        }
        if (mProfileLayout != null) {
            mProfileLayout.setAlpha(1f);
        }
    }
}
