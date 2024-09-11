/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.adapters;

import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;
import androidx.viewpager2.adapter.FragmentStateAdapter;

import org.chromium.brave_wallet.mojom.SignMessageRequest;
import org.chromium.chrome.browser.crypto_wallet.fragments.dapps.DAppsMessageFragment;

import java.util.List;

public class SignMessagePagerAdapter extends FragmentStateAdapter {
    private List<String> mTabTitles;
    private SignMessageRequest mCurrentSignMessageRequest;

    public SignMessagePagerAdapter(
            Fragment fragment,
            List<String> tabTitles,
            SignMessageRequest currentSignMessageRequest) {
        super(fragment);
        mTabTitles = tabTitles;
        mCurrentSignMessageRequest = currentSignMessageRequest;
    }

    @NonNull
    @Override
    public Fragment createFragment(int position) {
        // It could be we have another tab in the future
        switch (position) {
            case 0:
                return new DAppsMessageFragment(mCurrentSignMessageRequest);
        }
        throw new RuntimeException("Invalid fragment position " + position);
    }

    @Override
    public int getItemCount() {
        return mTabTitles.size();
    }
}
