/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.viewpager2.widget.ViewPager2;

import com.google.android.material.tabs.TabLayout;
import com.google.android.material.tabs.TabLayoutMediator;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.adapters.PortfolioFragmentStateAdapter;

/**
 * Main section of Brave Wallet showing Asset list and NFT grid view.
 */
public class PortfolioFragment extends Fragment {
    private TabLayout mTabLayout;
    private ViewPager2 mViewPager;

    private PortfolioFragmentStateAdapter mViewPagerAdapter;

    public static PortfolioFragment newInstance() {
        return new PortfolioFragment();
    }

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        return inflater.inflate(org.chromium.chrome.R.layout.fragment_portfolio, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        mTabLayout = view.findViewById(R.id.portfolio_tab_layout);
        mViewPager = view.findViewById(R.id.portfolio_view_pager);

        mViewPagerAdapter = new PortfolioFragmentStateAdapter(this);
        mViewPager.setAdapter(mViewPagerAdapter);
        mViewPager.setUserInputEnabled(false);
        new TabLayoutMediator(mTabLayout, mViewPager,
                (tab, position) -> tab.setText(mViewPagerAdapter.getPageTitle(position)))
                .attach();
    }

    public void callAnotherApproveDialog() {
        mViewPagerAdapter.getAssetsFragment().callAnotherApproveDialog();
    }
}
