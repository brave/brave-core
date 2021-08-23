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
import android.widget.TextView;

import androidx.preference.DialogPreference;
import androidx.recyclerview.widget.DividerItemDecoration;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.settings.BravePreferenceFragment;
import org.chromium.chrome.browser.vpn.BraveVpnServerSelectionAdapter;
import org.chromium.chrome.browser.vpn.BraveVpnUtils;
import org.chromium.chrome.browser.vpn.VpnServerRegion;

import java.util.Collections;
import java.util.Comparator;

public class BraveVpnServerSelectionPreferences extends BravePreferenceFragment {
    public static final String PREF_SERVER_CHANGE_LOCATION = "server_change_location";
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
        if (BraveVpnUtils.getServerRegion(PREF_SERVER_CHANGE_LOCATION).equals("automatic")) {
            automaticText.setCompoundDrawablesWithIntrinsicBounds(
                    0, 0, R.drawable.ic_server_selection_check, 0);
        } else {
            automaticText.setCompoundDrawablesWithIntrinsicBounds(0, 0, 0, 0);
        }
        automaticText.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                BraveVpnUtils.setServerRegion(PREF_SERVER_CHANGE_LOCATION, "automatic");
                getActivity().onBackPressed();
            }
        });

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
        Collections.sort(BraveVpnUtils.vpnServerRegions, new Comparator<VpnServerRegion>() {
            @Override
            public int compare(VpnServerRegion vpnServerRegion1, VpnServerRegion vpnServerRegion2) {
                return vpnServerRegion1.getNamePretty().compareToIgnoreCase(
                        vpnServerRegion2.getNamePretty());
            }
        });
        BraveVpnServerSelectionAdapter braveVpnServerSelectionAdapter =
                new BraveVpnServerSelectionAdapter();
        braveVpnServerSelectionAdapter.setVpnServerRegions(BraveVpnUtils.vpnServerRegions);
        braveVpnServerSelectionAdapter.setOnServerRegionSelection(onServerRegionSelection);
        serverRegionList.setAdapter(braveVpnServerSelectionAdapter);
        serverRegionList.setLayoutManager(linearLayoutManager);

        super.onActivityCreated(savedInstanceState);
    }

    OnServerRegionSelection onServerRegionSelection = new OnServerRegionSelection() {
        @Override
        public void onServerRegionClick(VpnServerRegion vpnServerRegion) {
            BraveVpnUtils.setServerRegion(PREF_SERVER_CHANGE_LOCATION, vpnServerRegion.getName());
            getActivity().onBackPressed();
        }
    };
}
