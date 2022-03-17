/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments.dapps;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.viewpager2.widget.ViewPager2;

import com.google.android.material.bottomsheet.BottomSheetDialogFragment;
import com.google.android.material.tabs.TabLayout;
import com.google.android.material.tabs.TabLayoutMediator;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.adapters.SignMessagePagerAdapter;

import java.util.ArrayList;
import java.util.List;

public class SignMessageFragment extends BottomSheetDialogFragment {
    private List<String> mTabTitles;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mTabTitles = new ArrayList<>();
        mTabTitles.add(getString(R.string.message));
        mTabTitles.add(getString(R.string.data_text));
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.fragment_sign_message, container, false);
        ViewPager2 viewPager = view.findViewById(R.id.fragment_sign_msg_tv_message_view_pager);
        TabLayout tabLayout = view.findViewById(R.id.fragment_sign_msg_tv_message_tabs);
        viewPager.setUserInputEnabled(false);

        SignMessagePagerAdapter adapter = new SignMessagePagerAdapter(this, mTabTitles);

        viewPager.setAdapter(adapter);
        viewPager.setOffscreenPageLimit(adapter.getItemCount() - 1);
        new TabLayoutMediator(
                tabLayout, viewPager, (tab, position) -> tab.setText(mTabTitles.get(position)))
                .attach();
        return view;
    }
}
