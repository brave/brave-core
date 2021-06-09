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

import org.chromium.chrome.browser.crypto_wallet.fragments.CryptoChildFragment;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class CryptoFragmentPageAdapter extends FragmentPagerAdapter {
    private final List<String> titles =
            new ArrayList<>(Arrays.asList("PORTFOLIO", "PRICES", "DEFI", "NFTS", "ACCOUNTS"));
    public CryptoFragmentPageAdapter(FragmentManager fm) {
        super(fm, BEHAVIOR_RESUME_ONLY_CURRENT_FRAGMENT);
    }

    @NonNull
    @Override
    public Fragment getItem(int position) {
        return CryptoChildFragment.newInstance();
    }

    @Override
    public int getCount() {
        return titles.size();
    }

    @Nullable
    @Override
    public CharSequence getPageTitle(int position) {
        return titles.get(position);
    }
}
