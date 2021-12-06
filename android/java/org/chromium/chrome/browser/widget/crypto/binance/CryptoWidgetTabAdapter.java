/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.widget.crypto.binance;

import android.content.Context;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.widget.NestedScrollView;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentStatePagerAdapter;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.widget.crypto.binance.CryptoWidgetBottomSheetDialogFragment;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class CryptoWidgetTabAdapter extends FragmentStatePagerAdapter {
    Context mContext = ContextUtils.getApplicationContext();
    private CryptoWidgetBottomSheetDialogFragment.BinanceBottomSheetListener mBinanceBottomSheetListener;
    List<String> titles = Arrays.asList(mContext.getResources().getString(R.string.summary),
            mContext.getResources().getString(R.string.deposit),
            mContext.getResources().getString(R.string.convert),
            mContext.getResources().getString(R.string.buy));

    public CryptoWidgetTabAdapter(@NonNull FragmentManager fm, int behavior) {
        super(fm, behavior);
    }

    public void setBinanceBottomSheetListener(CryptoWidgetBottomSheetDialogFragment.BinanceBottomSheetListener binanceBottomSheetListener) {
        mBinanceBottomSheetListener = binanceBottomSheetListener;
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
            ((BinanceConvertFragment)fragment).setBinanceBottomSheetListener(mBinanceBottomSheetListener);
        } else if (position == 3) {
            fragment = new BinanceBuyFragment();
            ((BinanceBuyFragment) fragment)
                    .setBinanceBottomSheetListener(mBinanceBottomSheetListener);
        }
        return fragment;
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

    @Override
    public void setPrimaryItem(@NonNull ViewGroup container, int position, @NonNull Object object) {
        super.setPrimaryItem(container, position, object);
        for (int i = 0; i < getCount(); i++) {
            if ((NestedScrollView) container.findViewById(R.id.summary_scrollview) != null) {
                ((NestedScrollView) container.findViewById(R.id.summary_scrollview))
                        .setNestedScrollingEnabled(false);
            } else if ((NestedScrollView) container.findViewById(R.id.deposit_scrollview) != null) {
                ((NestedScrollView) container.findViewById(R.id.deposit_scrollview))
                        .setNestedScrollingEnabled(false);
            }
        }

        Fragment currentFragment = (Fragment) object;
        if (currentFragment.getView() != null) {
            if (currentFragment.getClass() == BinanceSummaryFragment.class) {
                NestedScrollView currentNestedScrollView =
                        (NestedScrollView) currentFragment.getView().findViewById(
                                R.id.summary_scrollview);
                currentNestedScrollView.setNestedScrollingEnabled(true);
            } else if (currentFragment.getClass() == BinanceDepositFragment.class) {
                NestedScrollView currentNestedScrollView =
                        (NestedScrollView) currentFragment.getView().findViewById(
                                R.id.deposit_scrollview);
                currentNestedScrollView.setNestedScrollingEnabled(true);
            }
        }

        container.requestLayout();
    }
}