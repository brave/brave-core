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
import java.util.Collections;
import java.util.List;

public class CryptoWalletOnboardingPagerAdapter extends FragmentStatePagerAdapter {
    private List<NavigationItem> mNavigationItems = new ArrayList<>();

    public void setNavigationItems(List<NavigationItem> mNavigationItems) {
        this.mNavigationItems = mNavigationItems;
    }

    public void replaceWithNavigationItem(NavigationItem navigationItem, int index) {
        replaceWithNavigationItems(Collections.singletonList(navigationItem), index);
    }

    public void replaceWithNavigationItems(List<NavigationItem> navigationItems, int index) {
        this.mNavigationItems = this.mNavigationItems.subList(0, index);
        this.mNavigationItems.addAll(navigationItems);
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
        return mNavigationItems.get(position).getFragment();
    }

    @Override
    public int getCount() {
        return mNavigationItems.size();
    }

    @Nullable
    @Override
    public CharSequence getPageTitle(int position) {
        return mNavigationItems.get(position).getTitle();
    }
}
