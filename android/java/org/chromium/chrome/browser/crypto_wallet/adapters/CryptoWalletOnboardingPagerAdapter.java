/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.adapters;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentStatePagerAdapter;

import org.chromium.chrome.browser.crypto_wallet.util.NavigationItem;

import java.util.ArrayList;
import java.util.List;

public class CryptoWalletOnboardingPagerAdapter extends FragmentStatePagerAdapter {
    private List<NavigationItem> navigationItems = new ArrayList<>();

    public void setNavigationItems(List<NavigationItem> navigationItems) {
        this.navigationItems = navigationItems;
    }

    public void replaceWithNavigationItem(NavigationItem navigationItem, int index) {
        this.navigationItems =
                new ArrayList<NavigationItem>(this.navigationItems.subList(0, index));
        this.navigationItems.add(navigationItem);
    }

    public void replaceWithNavigationItems(List<NavigationItem> navigationItems, int index) {
        this.navigationItems =
                new ArrayList<NavigationItem>(this.navigationItems.subList(0, index));
        this.navigationItems.addAll(navigationItems);
    }

    public CryptoWalletOnboardingPagerAdapter(FragmentManager fm) {
        super(fm, BEHAVIOR_RESUME_ONLY_CURRENT_FRAGMENT);
    }

    @Override
    public int getItemPosition(@NonNull Object object) {
        return POSITION_NONE;
    }

    @NonNull
    @Override
    public Fragment getItem(int position) {
        return navigationItems.get(position).getFragment();
    }

    @Override
    public int getCount() {
        return navigationItems.size();
    }

    @Nullable
    @Override
    public CharSequence getPageTitle(int position) {
        return navigationItems.get(position).getTitle();
    }
}
