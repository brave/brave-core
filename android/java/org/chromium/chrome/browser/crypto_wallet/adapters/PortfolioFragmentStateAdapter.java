/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.adapters;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.viewpager2.adapter.FragmentStateAdapter;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.fragments.AssetsFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.NftGridFragment;

import java.util.ArrayList;
import java.util.Arrays;

/**
 * Adapter of Portfolio section showing assets and NFTs.
 * @see org.chromium.chrome.browser.crypto_wallet.fragments.PortfolioFragment
 */
public class PortfolioFragmentStateAdapter extends FragmentStateAdapter {
    private static final int ASSETS_FRAGMENT_POSITION = 0;
    private static final int NFT_GRID_FRAGMENT_POSITION = 1;

    private final ArrayList<String> mTitles;

    private AssetsFragment mAssetsFragment;
    private NftGridFragment mNftGridFragment;

    public PortfolioFragmentStateAdapter(@NonNull final Fragment fragment) {
        super(fragment);
        mTitles = new ArrayList<>(Arrays.asList(fragment.getString(R.string.assets),
                fragment.getString(R.string.brave_wallet_nfts)));
    }

    @NonNull
    @Override
    public Fragment createFragment(final int position) {
        switch (position) {
            case ASSETS_FRAGMENT_POSITION:
                if (mAssetsFragment == null) {
                    mAssetsFragment = AssetsFragment.newInstance();
                }
                return mAssetsFragment;
            case NFT_GRID_FRAGMENT_POSITION:
                if (mNftGridFragment == null) {
                    mNftGridFragment = NftGridFragment.newInstance();
                }
                return mNftGridFragment;
            default:
                throw new IllegalStateException(
                        String.format("No fragment found for position %d.", position));
        }
    }

    @Override
    public int getItemCount() {
        return mTitles.size();
    }

    @Nullable
    public CharSequence getPageTitle(final int position) {
        return mTitles.get(position);
    }
}
