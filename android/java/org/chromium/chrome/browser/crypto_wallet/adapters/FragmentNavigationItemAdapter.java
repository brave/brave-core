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

public class FragmentNavigationItemAdapter extends FragmentStateAdapter {
    private List<NavigationItem> navigationItems = new ArrayList<>();

    public FragmentNavigationItemAdapter(
            @NonNull FragmentActivity fragmentActivity, List<NavigationItem> navigationItems) {
        super(fragmentActivity);
        setNavigationItems(navigationItems);
    }

    public FragmentNavigationItemAdapter(
            @NonNull Fragment fragment, List<NavigationItem> navigationItems) {
        super(fragment);
        setNavigationItems(navigationItems);
    }

    public void setNavigationItems(List<NavigationItem> navigationItems) {
        this.navigationItems = navigationItems;
    }

    public void replaceWithNavigationItem(NavigationItem navigationItem, int index) {
        this.navigationItems = new ArrayList<>(this.navigationItems.subList(0, index));
        this.navigationItems.add(navigationItem);
    }

    public void replaceWithNavigationItems(List<NavigationItem> navigationItems, int index) {
        this.navigationItems =
                new ArrayList<NavigationItem>(this.navigationItems.subList(0, index));
        this.navigationItems.addAll(navigationItems);
    }

    @NonNull
    @Override
    public Fragment createFragment(int position) {
        return navigationItems.get(position).getFragment();
    }

    @Override
    public int getItemCount() {
        return navigationItems.size();
    }
}
