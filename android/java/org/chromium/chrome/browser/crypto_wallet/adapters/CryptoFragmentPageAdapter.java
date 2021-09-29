/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.adapters;

import android.app.Activity;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentStatePagerAdapter;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.fragments.AccountsFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.PortfolioFragment;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class CryptoFragmentPageAdapter extends FragmentStatePagerAdapter {
    private List<String> mTitles;

    public CryptoFragmentPageAdapter(FragmentManager fm, Activity activity) {
        super(fm, BEHAVIOR_RESUME_ONLY_CURRENT_FRAGMENT);
        mTitles = new ArrayList<>(Arrays.asList(activity.getText(R.string.portfolio).toString(),
                activity.getText(R.string.apps).toString(),
                activity.getText(R.string.accounts).toString()));
    }

    @NonNull
    @Override
    public Fragment getItem(int position) {
        if (position == 2) {
            return AccountsFragment.newInstance();
        } else {
            return PortfolioFragment.newInstance();
        }
    }

    @Override
    public int getCount() {
        return mTitles.size();
    }

    @Nullable
    @Override
    public CharSequence getPageTitle(int position) {
        return mTitles.get(position);
    }
}
