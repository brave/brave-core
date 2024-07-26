/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.vpn.activities;

import android.graphics.Rect;
import android.view.MenuItem;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.TextView;

import androidx.appcompat.app.ActionBar;
import androidx.appcompat.widget.Toolbar;
import androidx.recyclerview.widget.DividerItemDecoration;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.vpn.BraveVpnNativeWorker;
import org.chromium.chrome.browser.vpn.adapters.BraveVpnServerSelectionAdapter;
import org.chromium.chrome.browser.vpn.models.BraveVpnPrefModel;
import org.chromium.chrome.browser.vpn.models.BraveVpnServerRegion;
import org.chromium.chrome.browser.vpn.utils.BraveVpnPrefUtils;
import org.chromium.chrome.browser.vpn.utils.BraveVpnUtils;
import org.chromium.ui.widget.Toast;

import java.util.Collections;
import java.util.Comparator;
import java.util.List;

public class VpnServerSelectionActivity extends BraveVpnParentActivity {
    private BraveVpnServerSelectionAdapter mBraveVpnServerSelectionAdapter;
    private LinearLayout mServerSelectionListLayout;
    private ProgressBar mServerSelectionProgress;
    private RecyclerView mServerRegionList;

    public interface OnServerRegionSelection {
        void onServerRegionClick(BraveVpnServerRegion vpnServerRegion);
    }

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
        setContentView(R.layout.activity_vpn_server_selection);

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        ActionBar actionBar = getSupportActionBar();
        assert actionBar != null;
        actionBar.setDisplayHomeAsUpEnabled(true);
        actionBar.setTitle(getResources().getString(R.string.change_location));

        TextView automaticText = (TextView) findViewById(R.id.automatic_server_text);
        automaticText.setText(getResources().getString(R.string.automatic));
        if (BraveVpnPrefUtils.getServerRegion()
                .equals(BraveVpnPrefUtils.PREF_BRAVE_VPN_AUTOMATIC)) {
            automaticText.setCompoundDrawablesWithIntrinsicBounds(
                    0, 0, R.drawable.ic_checkbox_checked, 0);
        } else {
            automaticText.setCompoundDrawablesWithIntrinsicBounds(0, 0, 0, 0);
        }
        automaticText.setOnClickListener(
                new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        changeServerRegion(
                                new BraveVpnServerRegion(
                                        "", "", BraveVpnPrefUtils.PREF_BRAVE_VPN_AUTOMATIC, ""));
                    }
                });

        mServerSelectionListLayout = (LinearLayout) findViewById(R.id.server_selection_list_layout);
        mServerSelectionProgress = (ProgressBar) findViewById(R.id.server_selection_progress);

        mServerRegionList = (RecyclerView) findViewById(R.id.server_selection_list);
        LinearLayoutManager linearLayoutManager = new LinearLayoutManager(this);
        mServerRegionList.addItemDecoration(
                new DividerItemDecoration(
                        VpnServerSelectionActivity.this, linearLayoutManager.getOrientation()) {
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
        BraveVpnNativeWorker.getInstance().getServerRegionsWithCities();
        showProgress();
    }

    @Override
    public void onGetServerRegionsWithCities(String jsonResponse, boolean isSuccess) {
        if (isSuccess) {
            Log.e("brave_vpn", jsonResponse);
            List<BraveVpnServerRegion> braveVpnServerRegions =
                    BraveVpnUtils.getServerLocations(jsonResponse);
            Collections.sort(
                    braveVpnServerRegions,
                    new Comparator<BraveVpnServerRegion>() {
                        @Override
                        public int compare(
                                BraveVpnServerRegion braveVpnServerRegion1,
                                BraveVpnServerRegion braveVpnServerRegion2) {
                            return braveVpnServerRegion1
                                    .getNamePretty()
                                    .compareToIgnoreCase(braveVpnServerRegion2.getNamePretty());
                        }
                    });
            mBraveVpnServerSelectionAdapter = new BraveVpnServerSelectionAdapter();
            mBraveVpnServerSelectionAdapter.setVpnServerRegions(braveVpnServerRegions);
            mBraveVpnServerSelectionAdapter.setOnServerRegionSelection(
                    new OnServerRegionSelection() {
                        @Override
                        public void onServerRegionClick(BraveVpnServerRegion braveVpnServerRegion) {
                            if (BraveVpnPrefUtils.getServerRegion()
                                    .equals(braveVpnServerRegion.getName())) {
                                Toast.makeText(
                                                VpnServerSelectionActivity.this,
                                                R.string.already_selected_the_server,
                                                Toast.LENGTH_SHORT)
                                        .show();
                            } else {
                                changeServerRegion(braveVpnServerRegion);
                            }
                        }
                    });
            mServerRegionList.setAdapter(mBraveVpnServerSelectionAdapter);
            hideProgress();
        } else {
            Toast.makeText(
                            VpnServerSelectionActivity.this,
                            R.string.fail_to_get_server_locations,
                            Toast.LENGTH_LONG)
                    .show();
        }
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

    private void changeServerRegion(BraveVpnServerRegion braveVpnServerRegion) {
        mIsServerLocationChanged = true;
        BraveVpnUtils.selectedServerRegion = braveVpnServerRegion;
        BraveVpnUtils.showProgressDialog(
                VpnServerSelectionActivity.this,
                getResources().getString(R.string.vpn_connect_text));
        if (BraveVpnNativeWorker.getInstance().isPurchasedUser()) {
            mBraveVpnPrefModel = new BraveVpnPrefModel();
            BraveVpnNativeWorker.getInstance().getSubscriberCredentialV12();
        } else {
            verifySubscription();
        }
    }

    public void showProgress() {
        if (mServerSelectionProgress != null) {
            mServerSelectionProgress.setVisibility(View.VISIBLE);
        }
        if (mServerSelectionListLayout != null) {
            mServerSelectionListLayout.setAlpha(0.4f);
        }
    }

    public void hideProgress() {
        if (mServerSelectionProgress != null) {
            mServerSelectionProgress.setVisibility(View.GONE);
        }
        if (mServerSelectionListLayout != null) {
            mServerSelectionListLayout.setAlpha(1f);
        }
    }

    @Override
    public void showRestoreMenu(boolean shouldShowRestore) {}

    @Override
    public void updateProfileView() {}
}
