/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.adapters;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentPagerAdapter;

import java.util.ArrayList;
import java.util.List;

import org.chromium.chrome.browser.crypto_wallet.util.NavigationItem;

public class WalletNavigationFragmentPageAdapter extends FragmentPagerAdapter {
    private List<NavigationItem> navigationItems = new ArrayList<>();

    public void setNavigationItems(List<NavigationItem> navigationItems) {
        this.navigationItems = navigationItems;
    }

    public WalletNavigationFragmentPageAdapter(FragmentManager fm) {
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
