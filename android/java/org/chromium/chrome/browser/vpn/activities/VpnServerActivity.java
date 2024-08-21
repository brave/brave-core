/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.vpn.activities;

import android.graphics.Rect;
import android.view.MenuItem;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.ProgressBar;

import androidx.appcompat.app.ActionBar;
import androidx.appcompat.widget.Toolbar;
import androidx.recyclerview.widget.DividerItemDecoration;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.brave_vpn.mojom.BraveVpnConstants;
import org.chromium.brave_vpn.mojom.Region;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.vpn.adapters.VpnServerAdapter;
import org.chromium.chrome.browser.vpn.models.BraveVpnServerRegion;
import org.chromium.chrome.browser.vpn.utils.BraveVpnPrefUtils;
import org.chromium.chrome.browser.vpn.utils.BraveVpnUtils;
import org.chromium.ui.widget.Toast;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class VpnServerActivity extends BraveVpnParentActivity {
    private VpnServerAdapter mVpnServerAdapter;

    private LinearLayout mServerListLayout;
    private ProgressBar mServerProgress;
    private RecyclerView mServerRegionList;

    public interface OnCitySelection {
        void onCityClick(Region city, int position);
    }

    private void initializeViews() {
        setContentView(R.layout.activity_vpn_server);

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        ActionBar actionBar = getSupportActionBar();
        assert actionBar != null;
        actionBar.setDisplayHomeAsUpEnabled(true);
        actionBar.setTitle(getResources().getString(R.string.change_location));

        mServerRegionList = (RecyclerView) findViewById(R.id.server_list);

        mServerListLayout = (LinearLayout) findViewById(R.id.server_layout);
        mServerProgress = (ProgressBar) findViewById(R.id.server_progress);

        LinearLayoutManager linearLayoutManager = new LinearLayoutManager(this);
        mServerRegionList.addItemDecoration(
                new DividerItemDecoration(
                        VpnServerActivity.this, linearLayoutManager.getOrientation()) {
                    @Override
                    public void getItemOffsets(
                            Rect outRect,
                            View view,
                            RecyclerView parent,
                            RecyclerView.State state) {
                        int position = parent.getChildAdapterPosition(view);
                        // hide the divider for the last child
                        if (position == state.getItemCount() - 1) {
                            outRect.setEmpty();
                        } else {
                            super.getItemOffsets(outRect, view, parent, state);
                        }
                    }
                });
        mServerRegionList.setLayoutManager(linearLayoutManager);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item.getItemId() == android.R.id.home) {
            finish();
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    public void finishNativeInitialization() {
        super.finishNativeInitialization();
        Region region = BraveVpnUtils.selectedRegion;
        showProgress();
        List<Region> cities = new ArrayList<Region>(Arrays.asList(region.cities));

        Region optimalRegion = new Region();
        optimalRegion.name = getString(R.string.optimal_desc);
        optimalRegion.namePretty = getString(R.string.optimal_text);
        cities.add(0, optimalRegion);

        mVpnServerAdapter = new VpnServerAdapter(VpnServerActivity.this);
        mVpnServerAdapter.setCities(cities);
        mVpnServerAdapter.setRegion(region);
        mVpnServerAdapter.setOnCitySelection(
                new OnCitySelection() {
                    @Override
                    public void onCityClick(Region city, int position) {
                        if (BraveVpnPrefUtils.getRegionName().equals(city.name)) {
                            Toast.makeText(
                                            VpnServerActivity.this,
                                            R.string.already_selected_the_server,
                                            Toast.LENGTH_SHORT)
                                    .show();
                        } else {
                            BraveVpnUtils.selectedServerRegion =
                                    new BraveVpnServerRegion(
                                            region.countryIsoCode,
                                            region.name,
                                            region.namePretty,
                                            city.name,
                                            city.namePretty,
                                            position == 0
                                                    ? BraveVpnConstants.REGION_PRECISION_COUNTRY
                                                    : BraveVpnConstants.REGION_PRECISION_CITY);
                            changeServerRegion();
                        }
                    }
                });
        mServerRegionList.setAdapter(mVpnServerAdapter);
        hideProgress();
    }

    @Override
    protected void triggerLayoutInflation() {
        initializeViews();
        onInitialLayoutInflationComplete();
    }

    @Override
    public boolean shouldStartGpuProcess() {
        return true;
    }

    public void showProgress() {
        if (mServerProgress != null) {
            mServerProgress.setVisibility(View.VISIBLE);
        }
        if (mServerListLayout != null) {
            mServerListLayout.setAlpha(0.4f);
        }
    }

    public void hideProgress() {
        if (mServerProgress != null) {
            mServerProgress.setVisibility(View.GONE);
        }
        if (mServerListLayout != null) {
            mServerListLayout.setAlpha(1f);
        }
    }

    @Override
    public void showRestoreMenu(boolean shouldShowRestore) {}

    @Override
    public void updateProfileView() {}
}
