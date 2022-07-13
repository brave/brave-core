/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.vpn.split_tunnel;

import android.os.Bundle;
import android.os.Handler;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.widget.Toolbar;
import androidx.core.widget.NestedScrollView;
import androidx.lifecycle.LifecycleOwner;
import androidx.lifecycle.ViewModelProvider;
import androidx.recyclerview.widget.DividerItemDecoration;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.firstrun.BraveFirstRunFlowSequencer;
import org.chromium.chrome.browser.init.AsyncInitializationActivity;
import org.chromium.chrome.browser.vpn.BraveVpnNativeWorker;
import org.chromium.chrome.browser.vpn.utils.BraveVpnPrefUtils;
import org.chromium.chrome.browser.vpn.utils.BraveVpnUtils;

public class SplitTunnelActivity extends AsyncInitializationActivity
        implements LifecycleOwner, ApplicationListAdapter.OnApplicationClickListener {
    private BraveFirstRunFlowSequencer mFirstRunFlowSequencer;
    private RecyclerView mRecyclerViewSystemApps;
    private ApplicationListAdapter mRecyclerViewAdapterExcludedApps;
    private ApplicationListAdapter mRecyclerViewAdapterApps;
    private ApplicationListAdapter mRecyclerViewAdapterSystemApps;

    private void initializeViews() {
        setContentView(R.layout.activity_split_tunnel);

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        ActionBar actionBar = getSupportActionBar();
        assert actionBar != null;
        actionBar.setDisplayHomeAsUpEnabled(true);
        actionBar.setTitle(getResources().getString(R.string.split_tunneling));

        LinearLayoutManager linearLayoutManager = new LinearLayoutManager(this);
        linearLayoutManager.setOrientation(LinearLayoutManager.VERTICAL);

        DividerItemDecoration dividerItemDecoration =
                new DividerItemDecoration(SplitTunnelActivity.this, LinearLayoutManager.VERTICAL);

        RecyclerView recyclerViewExcludedApps = findViewById(R.id.rv_main1);
        recyclerViewExcludedApps.setLayoutManager(linearLayoutManager);
        recyclerViewExcludedApps.addItemDecoration(dividerItemDecoration);
        mRecyclerViewAdapterExcludedApps = new ApplicationListAdapter(this, true);
        recyclerViewExcludedApps.setAdapter(mRecyclerViewAdapterExcludedApps);

        LinearLayoutManager linearLayoutManager2 = new LinearLayoutManager(this);
        linearLayoutManager.setOrientation(LinearLayoutManager.VERTICAL);

        RecyclerView recyclerViewApps = findViewById(R.id.rv_main2);
        recyclerViewApps.setLayoutManager(linearLayoutManager2);
        recyclerViewApps.addItemDecoration(dividerItemDecoration);
        mRecyclerViewAdapterApps = new ApplicationListAdapter(this, false);
        recyclerViewApps.setAdapter(mRecyclerViewAdapterApps);

        LinearLayoutManager linearLayoutManager3 = new LinearLayoutManager(this);
        linearLayoutManager3.setOrientation(LinearLayoutManager.VERTICAL);

        mRecyclerViewSystemApps = findViewById(R.id.rv_system);
        mRecyclerViewSystemApps.setLayoutManager(linearLayoutManager3);
        mRecyclerViewSystemApps.addItemDecoration(dividerItemDecoration);
        mRecyclerViewAdapterSystemApps = new ApplicationListAdapter(this, false);
        mRecyclerViewSystemApps.setAdapter(mRecyclerViewAdapterSystemApps);

        NestedScrollView appsLayout = findViewById(R.id.apps_layout);
        ProgressBar progressBar = findViewById(R.id.progressbar);
        progressBar.setVisibility(View.VISIBLE);
        appsLayout.setVisibility(View.GONE);

        Button showSystemAppsBtn = findViewById(R.id.show_system_app_btn);
        showSystemAppsBtn.setOnClickListener(view -> {
            showSystemAppsBtn.setVisibility(View.GONE);
            mRecyclerViewSystemApps.setVisibility(View.VISIBLE);
        });

        ApplicationViewModel viewModel =
                new ViewModelProvider(SplitTunnelActivity.this).get(ApplicationViewModel.class);
        viewModel.getApplications(SplitTunnelActivity.this);

        viewModel.getExcludedApplicationDataLiveData().observe(
                SplitTunnelActivity.this, applicationDataModels -> {
                    if (applicationDataModels.size() > 0) {
                        mRecyclerViewAdapterExcludedApps.addAll(applicationDataModels);
                    } else {
                        TextView emptyExcludedAppsText =
                                findViewById(R.id.empty_excluded_apps_text);
                        emptyExcludedAppsText.setVisibility(View.VISIBLE);
                    }
                });
        viewModel.getApplicationDataMutableLiveData().observe(
                SplitTunnelActivity.this, applicationDataModels -> {
                    mRecyclerViewAdapterApps.addAll(applicationDataModels);
                    appsLayout.setVisibility(View.VISIBLE);
                    progressBar.setVisibility(View.GONE);
                });
        viewModel.getSystemApplicationDataMutableLiveData().observe(SplitTunnelActivity.this,
                applicationDataModels
                -> mRecyclerViewAdapterSystemApps.addAll(applicationDataModels));
    }

    @Override
    protected void triggerLayoutInflation() {
        mFirstRunFlowSequencer = new BraveFirstRunFlowSequencer(this) {
            @Override
            public void onFlowIsKnown(Bundle freProperties) {
                initializeViews();
            }
        };
        mFirstRunFlowSequencer.start();
        onInitialLayoutInflationComplete();
    }

    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.split_tunnel_menu, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(@NonNull MenuItem item) {
        if (item.getItemId() == R.id.action_save) {
            if (mRecyclerViewAdapterExcludedApps != null) {
                BraveVpnPrefUtils.setExcludedPackages(
                        mRecyclerViewAdapterExcludedApps.getApplicationPackages());
                BraveVpnNativeWorker.getInstance().invalidateCredentials(
                        BraveVpnPrefUtils.getHostname(), BraveVpnPrefUtils.getClientId(),
                        BraveVpnPrefUtils.getSubscriberCredential(),
                        BraveVpnPrefUtils.getApiAuthToken());
                BraveVpnUtils.resetProfileConfiguration(SplitTunnelActivity.this);
                finish();
                return true;
            }
        } else if (item.getItemId() == android.R.id.home) {
            finish();
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    public void onApplicationCLick(
            ApplicationDataModel applicationDataModel, int position, boolean isExcludedApps) {
        if (mRecyclerViewAdapterExcludedApps != null && mRecyclerViewAdapterApps != null
                && mRecyclerViewAdapterSystemApps != null) {
            if (isExcludedApps) {
                mRecyclerViewAdapterExcludedApps.removeApplication(applicationDataModel);
                if (applicationDataModel.isSystemApp()) {
                    mRecyclerViewAdapterSystemApps.addApplication(applicationDataModel);
                } else {
                    mRecyclerViewAdapterApps.addApplication(applicationDataModel);
                }
            } else {
                mRecyclerViewAdapterExcludedApps.addApplication(applicationDataModel);
                if (applicationDataModel.isSystemApp()) {
                    mRecyclerViewAdapterSystemApps.removeApplication(applicationDataModel);
                } else {
                    mRecyclerViewAdapterApps.removeApplication(applicationDataModel);
                }
            }
        }
    }

    @Override
    public boolean shouldStartGpuProcess() {
        return true;
    }
}