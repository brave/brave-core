/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.widget.crypto.binance;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentStatePagerAdapter;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class CryptoWidgetTabAdapter extends FragmentStatePagerAdapter {
    List<String> titles = Arrays.asList("Summary", "Deposit", "Convert", "Buy");

    public CryptoWidgetTabAdapter(@NonNull FragmentManager fm, int behavior) {
        super(fm, behavior);
    }

    @NonNull
    @Override
    public Fragment getItem(int position) {
        Fragment fragment = null;
        if (position == 0) {
            fragment = new BinanceSummaryFragment();
        } else if (position == 1) {
            fragment = new BinanceDepositFragment();
        } else if (position == 2) {
            fragment = new BinanceConvertFragment();
        } else if(position == 3) {
            fragment = new BinanceBuyFragment();
        }
        return fragment;
    }

    @Override
    public int getCount() {
        return 4;
    }

    @Nullable
    @Override
    public CharSequence getPageTitle(int position) {
        return titles.get(position);
    }
}