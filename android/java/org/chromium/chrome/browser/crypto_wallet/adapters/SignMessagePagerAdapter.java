/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.adapters;

import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;
import androidx.viewpager2.adapter.FragmentStateAdapter;

import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.brave_wallet.mojom.TxData;
import org.chromium.brave_wallet.mojom.TxData1559;
import org.chromium.brave_wallet.mojom.TxDataUnion;
import org.chromium.chrome.browser.crypto_wallet.fragments.TxDetailsFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.dapps.DAppsMessageFragment;

import java.util.List;

public class SignMessagePagerAdapter extends FragmentStateAdapter {
    private List<String> mTabTitles;

    public SignMessagePagerAdapter(Fragment fragment, List<String> tabTitles) {
        super(fragment);
        mTabTitles = tabTitles;
    }

    @NonNull
    @Override
    public Fragment createFragment(int position) {
        switch (position) {
            case 0:
                return new DAppsMessageFragment();
            case 1: {
                // TODO: replace transactionInfo with required data param
                TransactionInfo transactionInfo = new TransactionInfo();
                transactionInfo.txParams = new String[] {};
                transactionInfo.txArgs = new String[] {};
                transactionInfo.txDataUnion = new TxDataUnion();

                TxData1559 txData1559 = new TxData1559();
                txData1559.baseData = new TxData();
                txData1559.baseData.data = new byte[] {};
                transactionInfo.txDataUnion.setEthTxData1559(txData1559);
                return TxDetailsFragment.newInstance(transactionInfo);
            }
        }
        throw new RuntimeException("Invalid fragment position " + position);
    }

    @Override
    public int getItemCount() {
        return mTabTitles.size();
    }
}
