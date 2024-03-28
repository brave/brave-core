/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.adapters;

import android.annotation.SuppressLint;

import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentActivity;
import androidx.viewpager2.adapter.FragmentStateAdapter;

import org.chromium.chrome.browser.crypto_wallet.fragments.BaseWalletNextPageFragment;

import java.util.ArrayList;
import java.util.List;

public class CryptoWalletOnboardingPagerAdapter extends FragmentStateAdapter {
    @NonNull private final List<BaseWalletNextPageFragment> mNavigationItems = new ArrayList<>();

    public CryptoWalletOnboardingPagerAdapter(@NonNull final FragmentActivity fragmentActivity) {
        super(fragmentActivity);
    }

    @SuppressLint("NotifyDataSetChanged")
    public void setNavigationItems(
            @NonNull final List<BaseWalletNextPageFragment> navigationFragments) {
        mNavigationItems.clear();
        mNavigationItems.addAll(navigationFragments);
        notifyDataSetChanged();
    }

    /**
     * Replaces all navigation items starting from a given index.
     *
     * @param navigationFragments Navigation fragments to add.
     * @param index Index pointing to the first item that will be replaced.
     */
    @SuppressLint("NotifyDataSetChanged")
    public void replaceWithNavigationItems(
            @NonNull final List<BaseWalletNextPageFragment> navigationFragments, final int index) {
        // Clear the list from the index (included).
        mNavigationItems.subList(index, mNavigationItems.size()).clear();
        // Append new navigation items to the list.
        mNavigationItems.addAll(navigationFragments);
        notifyDataSetChanged();
    }

    @NonNull
    @Override
    public Fragment createFragment(int position) {
        return mNavigationItems.get(position);
    }

    @Override
    public int getItemCount() {
        return mNavigationItems.size();
    }
}
