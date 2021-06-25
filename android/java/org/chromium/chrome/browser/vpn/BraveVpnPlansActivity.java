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
import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.appcompat.app.ActionBar;
import androidx.appcompat.widget.Toolbar;
import androidx.viewpager.widget.ViewPager;

import com.google.android.material.tabs.TabLayout;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.firstrun.FirstRunFlowSequencer;
import org.chromium.chrome.browser.init.AsyncInitializationActivity;
import org.chromium.chrome.browser.vpn.BraveVpnPlanPagerAdapter;
import org.chromium.chrome.browser.vpn.BraveVpnUtils;

public class BraveVpnPlansActivity extends AsyncInitializationActivity {
    private FirstRunFlowSequencer mFirstRunFlowSequencer;

    private void initializeViews() {
        setContentView(R.layout.activity_brave_vpn_plan);

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        ActionBar actionBar = getSupportActionBar();
        assert actionBar != null;
        actionBar.setDisplayHomeAsUpEnabled(true);
        actionBar.setHomeAsUpIndicator(R.drawable.ic_baseline_close_24);
        actionBar.setTitle(getResources().getString(R.string.brave_vpn));

        ViewPager braveRewardsViewPager = findViewById(R.id.brave_rewards_view_pager);

        BraveVpnPlanPagerAdapter braveVpnPlanPagerAdapter = new BraveVpnPlanPagerAdapter(this);
        braveRewardsViewPager.setAdapter(braveVpnPlanPagerAdapter);
        TabLayout braveRewardsTabLayout = findViewById(R.id.brave_rewards_tab_layout);
        braveRewardsTabLayout.setupWithViewPager(braveRewardsViewPager, true);

        TextView removedValueText = findViewById(R.id.removed_value_tv);
        removedValueText.setPaintFlags(
                removedValueText.getPaintFlags() | Paint.STRIKE_THRU_TEXT_FLAG);

        LinearLayout monthlySelectorLayout = findViewById(R.id.monthly_selector_layout);
        monthlySelectorLayout.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                BraveVpnUtils.openBraveVpnProfileActivity(BraveVpnPlansActivity.this);
                finish();
            }
        });

        LinearLayout yearlySelectorLayout = findViewById(R.id.yearly_selector_layout);
        yearlySelectorLayout.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                BraveVpnUtils.openBraveVpnProfileActivity(BraveVpnPlansActivity.this);
                finish();
            }
        });
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.menu_brave_vpn, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item.getItemId() == android.R.id.home) {
            finish();
        } else if (item.getItemId() == R.id.restore) {
            // Do nothing for now
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
}
