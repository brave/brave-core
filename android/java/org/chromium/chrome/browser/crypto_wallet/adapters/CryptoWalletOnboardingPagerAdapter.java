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
    private final List<NavigationItem> mNavigationItems = new ArrayList<>();

    public void setNavigationItems(@NonNull final List<NavigationItem> navigationItems) {
        mNavigationItems.clear();
        mNavigationItems.addAll(navigationItems);
    }

    /**
     * Replaces all navigation items starting from a given index.
     *
     * @param navigationItems Navigation items to add.
     * @param index Index pointing to the first item that will be replaced.
     */
    public void replaceWithNavigationItems(
            @NonNull final List<NavigationItem> navigationItems, final int index) {
        // Clear the list from the index (included).
        mNavigationItems.subList(index, mNavigationItems.size()).clear();
        // Append new navigation items to the list.
        mNavigationItems.addAll(navigationItems);
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
}
