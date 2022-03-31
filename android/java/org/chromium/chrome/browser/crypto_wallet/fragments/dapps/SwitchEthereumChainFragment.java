/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments.dapps;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.fragment.app.Fragment;
import androidx.viewpager2.widget.ViewPager2;

import com.google.android.material.tabs.TabLayout;
import com.google.android.material.tabs.TabLayoutMediator;

import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.adapters.FragmentNavigationItemAdapter;
import org.chromium.chrome.browser.crypto_wallet.adapters.TwoLineItemRecyclerViewAdapter.TwoLineItemDataSource;
import org.chromium.chrome.browser.crypto_wallet.fragments.TwoLineItemFragment;
import org.chromium.chrome.browser.crypto_wallet.util.NavigationItem;

import java.util.ArrayList;
import java.util.List;

public class SwitchEthereumChainFragment extends Fragment {
    private List<NavigationItem> mTabTitles;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mTabTitles = new ArrayList<>();
        // Todo: replace hardcoded strings with striung resources if required
        NetworkInfo info = new NetworkInfo();
        info.chainName = "Rinkeby Test Network";
        info.rpcUrls = new String[] {"Rinkeby Test Network"};
        info.chainId = "0x4";
        info.symbol = "ETH";
        info.decimals = 18;
        info.blockExplorerUrls = new String[] {"https://rinkeby.etherscan.io"};

        List<TwoLineItemDataSource> networks = new ArrayList<>();
        networks.add(new TwoLineItemDataSource("Network name", info.chainName));
        networks.add(new TwoLineItemDataSource(
                "Network URL", info.rpcUrls.length > 0 ? info.rpcUrls[0] : ""));

        List<TwoLineItemDataSource> details = new ArrayList<>();
        details.add(new TwoLineItemDataSource("Network name", "Rinkeby Test Network"));
        details.add(new TwoLineItemDataSource("Network URL", "rinkeyby-infura.brave.com"));
        details.add(new TwoLineItemDataSource("Chain ID", info.chainId));
        details.add(new TwoLineItemDataSource("Currency symbol", info.symbol));
        details.add(
                new TwoLineItemDataSource("Decimals of precision", String.valueOf(info.decimals)));
        details.add(new TwoLineItemDataSource("Block explorer URL",
                info.blockExplorerUrls.length > 0 ? info.blockExplorerUrls[0] : ""));

        mTabTitles.add(new NavigationItem("Network", new TwoLineItemFragment(networks)));
        mTabTitles.add(new NavigationItem("Details", new TwoLineItemFragment(details)));
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.fragment_switch_ethereum_chain, container, false);
        ViewPager2 viewPager =
                view.findViewById(R.id.fragment_switch_eth_chain_tv_message_view_pager);
        TabLayout tabLayout = view.findViewById(R.id.fragment_switch_eth_chain_tv_message_tabs);
        viewPager.setUserInputEnabled(false);

        FragmentNavigationItemAdapter adapter = new FragmentNavigationItemAdapter(this, mTabTitles);

        viewPager.setAdapter(adapter);
        new TabLayoutMediator(tabLayout, viewPager,
                (tab, position) -> tab.setText(mTabTitles.get(position).getTitle()))
                .attach();
        return view;
    }
}
