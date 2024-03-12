/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.adapters;

import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentActivity;
import androidx.viewpager2.adapter.FragmentStateAdapter;

import org.chromium.chrome.browser.crypto_wallet.util.NavigationItem;

import java.util.ArrayList;
import java.util.List;

public class CryptoWalletOnboardingPagerAdapter extends FragmentStateAdapter {
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

    public CryptoWalletOnboardingPagerAdapter(@NonNull final FragmentActivity fragmentActivity) {
        super(fragmentActivity);
    }

    @NonNull
    @Override
    public Fragment createFragment(int position) {
        return mNavigationItems.get(position).getFragment();
    }

    @Override
    public int getItemCount() {
        return mNavigationItems.size();
    }
}
