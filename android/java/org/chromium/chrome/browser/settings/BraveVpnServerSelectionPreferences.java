/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.content.Context;
import android.graphics.Rect;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.TextView;

import androidx.recyclerview.widget.DividerItemDecoration;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.settings.BravePreferenceFragment;
import org.chromium.chrome.browser.vpn.BraveVpnObserver;
import org.chromium.chrome.browser.vpn.BraveVpnPrefUtils;
import org.chromium.chrome.browser.vpn.BraveVpnServerRegion;
import org.chromium.chrome.browser.vpn.BraveVpnServerSelectionAdapter;
import org.chromium.chrome.browser.vpn.BraveVpnUtils;

import java.util.Collections;
import java.util.Comparator;
import java.util.List;

public class BraveVpnServerSelectionPreferences
        extends BravePreferenceFragment implements BraveVpnObserver {
    private BraveVpnServerSelectionAdapter mBraveVpnServerSelectionAdapter;
    private LinearLayout mServerSelectionListLayout;
    private ProgressBar mServerSelectionProgress;

    public interface OnServerRegionSelection {
        void onServerRegionClick(BraveVpnServerRegion vpnServerRegion);
    }
    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(R.layout.brave_vpn_server_selection_layout, container, false);
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        getActivity().setTitle(R.string.change_location);

        TextView automaticText = (TextView) getView().findViewById(R.id.automatic_server_text);
        automaticText.setText(getActivity().getResources().getString(R.string.automatic));
        if (BraveVpnPrefUtils.getServerRegion().equals(
                    BraveVpnPrefUtils.PREF_BRAVE_VPN_AUTOMATIC)) {
            automaticText.setCompoundDrawablesWithIntrinsicBounds(
                    0, 0, R.drawable.ic_server_selection_check, 0);
        } else {
            automaticText.setCompoundDrawablesWithIntrinsicBounds(0, 0, 0, 0);
        }
        automaticText.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                BraveVpnPrefUtils.setServerRegion(BraveVpnPrefUtils.PREF_BRAVE_VPN_AUTOMATIC);
                BraveVpnUtils.mIsServerLocationChanged = true;
                getActivity().onBackPressed();
            }
        });

        mServerSelectionListLayout =
                (LinearLayout) getView().findViewById(R.id.server_selection_list_layout);
        mServerSelectionProgress =
                (ProgressBar) getView().findViewById(R.id.server_selection_progress);

        LinearLayoutManager linearLayoutManager = new LinearLayoutManager(getContext());
        RecyclerView serverRegionList =
                (RecyclerView) getView().findViewById(R.id.server_selection_list);
        serverRegionList.addItemDecoration(
                new DividerItemDecoration(getActivity(), linearLayoutManager.getOrientation()) {
                    @Override
                    public void getItemOffsets(Rect outRect, View view, RecyclerView parent,
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
        List<BraveVpnServerRegion> braveVpnServerRegions =
                BraveVpnUtils.getServerLocations(BraveVpnPrefUtils.getServerRegions());
        Collections.sort(braveVpnServerRegions, new Comparator<BraveVpnServerRegion>() {
            @Override
            public int compare(BraveVpnServerRegion braveVpnServerRegion1,
                    BraveVpnServerRegion braveVpnServerRegion2) {
                return braveVpnServerRegion1.getNamePretty().compareToIgnoreCase(
                        braveVpnServerRegion2.getNamePretty());
            }
        });
        mBraveVpnServerSelectionAdapter = new BraveVpnServerSelectionAdapter();
        mBraveVpnServerSelectionAdapter.setVpnServerRegions(braveVpnServerRegions);
        mBraveVpnServerSelectionAdapter.setOnServerRegionSelection(onServerRegionSelection);
        serverRegionList.setAdapter(mBraveVpnServerSelectionAdapter);
        serverRegionList.setLayoutManager(linearLayoutManager);

        super.onActivityCreated(savedInstanceState);
    }

    OnServerRegionSelection onServerRegionSelection = new OnServerRegionSelection() {
        @Override
        public void onServerRegionClick(BraveVpnServerRegion braveVpnServerRegion) {
            BraveVpnPrefUtils.setServerRegion(braveVpnServerRegion.getName());
            BraveVpnUtils.mIsServerLocationChanged = true;
            getActivity().onBackPressed();
        }
    };

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
}
