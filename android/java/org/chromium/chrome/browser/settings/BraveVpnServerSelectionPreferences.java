/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.content.Context;
import android.graphics.Rect;
import android.os.Bundle;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.TextView;

import androidx.preference.DialogPreference;
import androidx.recyclerview.widget.DividerItemDecoration;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.settings.BravePreferenceFragment;
import org.chromium.chrome.browser.vpn.BraveVpnNativeWorker;
import org.chromium.chrome.browser.vpn.BraveVpnObserver;
import org.chromium.chrome.browser.vpn.BraveVpnPrefUtils;
import org.chromium.chrome.browser.vpn.BraveVpnServerSelectionAdapter;
import org.chromium.chrome.browser.vpn.BraveVpnUtils;
import org.chromium.chrome.browser.vpn.VpnServerRegion;
import org.chromium.ui.widget.Toast;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

public class BraveVpnServerSelectionPreferences
        extends BravePreferenceFragment implements BraveVpnObserver {
    public static final String PREF_SERVER_CHANGE_LOCATION = "server_change_location";
    private BraveVpnServerSelectionAdapter braveVpnServerSelectionAdapter;
    private LinearLayout serverSelectionListLayout;
    private ProgressBar serverSelectionProgress;

    public interface OnServerRegionSelection {
        void onServerRegionClick(VpnServerRegion vpnServerRegion);
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
        automaticText.setText("Automatic");
        if (BraveVpnPrefUtils.getServerRegion(PREF_SERVER_CHANGE_LOCATION).equals("automatic")) {
            automaticText.setCompoundDrawablesWithIntrinsicBounds(
                    0, 0, R.drawable.ic_server_selection_check, 0);
        } else {
            automaticText.setCompoundDrawablesWithIntrinsicBounds(0, 0, 0, 0);
        }
        automaticText.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                BraveVpnPrefUtils.setServerRegion(PREF_SERVER_CHANGE_LOCATION, "automatic");
                BraveVpnUtils.isServerLocationChanged = true;
                getActivity().onBackPressed();
            }
        });

        serverSelectionListLayout =
                (LinearLayout) getView().findViewById(R.id.server_selection_list_layout);
        serverSelectionProgress =
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
        List<VpnServerRegion> vpnServerRegions =
                BraveVpnUtils.getServerLocations(BraveVpnPrefUtils.getBraveVpnStringPref(
                        BraveVpnPrefUtils.PREF_BRAVE_VPN_SERVER_REGIONS));
        Collections.sort(vpnServerRegions, new Comparator<VpnServerRegion>() {
            @Override
            public int compare(VpnServerRegion vpnServerRegion1, VpnServerRegion vpnServerRegion2) {
                return vpnServerRegion1.getNamePretty().compareToIgnoreCase(
                        vpnServerRegion2.getNamePretty());
            }
        });
        braveVpnServerSelectionAdapter = new BraveVpnServerSelectionAdapter();
        braveVpnServerSelectionAdapter.setVpnServerRegions(vpnServerRegions);
        braveVpnServerSelectionAdapter.setOnServerRegionSelection(onServerRegionSelection);
        serverRegionList.setAdapter(braveVpnServerSelectionAdapter);
        serverRegionList.setLayoutManager(linearLayoutManager);

        super.onActivityCreated(savedInstanceState);
    }

    OnServerRegionSelection onServerRegionSelection = new OnServerRegionSelection() {
        @Override
        public void onServerRegionClick(VpnServerRegion vpnServerRegion) {
            BraveVpnPrefUtils.setServerRegion(
                    PREF_SERVER_CHANGE_LOCATION, vpnServerRegion.getName());
            BraveVpnUtils.isServerLocationChanged = true;
            getActivity().onBackPressed();
        }
    };

    public void showProgress() {
        if (serverSelectionProgress != null) {
            serverSelectionProgress.setVisibility(View.VISIBLE);
        }
        if (serverSelectionListLayout != null) {
            serverSelectionListLayout.setAlpha(0.4f);
        }
    }

    public void hideProgress() {
        if (serverSelectionProgress != null) {
            serverSelectionProgress.setVisibility(View.GONE);
        }
        if (serverSelectionListLayout != null) {
            serverSelectionListLayout.setAlpha(1f);
        }
    }
}
