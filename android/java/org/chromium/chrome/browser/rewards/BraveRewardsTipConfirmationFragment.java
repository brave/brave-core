
/**
 * Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.rewards;

import android.annotation.SuppressLint;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import androidx.fragment.app.Fragment;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveRewardsSiteBannerActivity;

public class BraveRewardsTipConfirmationFragment extends Fragment {
    public static BraveRewardsTipConfirmationFragment newInstance(
            double amount, boolean isMonthly) {
        BraveRewardsTipConfirmationFragment fragment = new BraveRewardsTipConfirmationFragment();
        Bundle args = new Bundle();
        args.putDouble(BraveRewardsSiteBannerActivity.AMOUNT_EXTRA, amount);
        args.putBoolean(BraveRewardsSiteBannerActivity.IS_MONTHLY_CONTRIBUTION, isMonthly);
        fragment.setArguments(args);
        return fragment;
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.brave_rewards_tip_confirmation, container, false);
        if (getArguments() != null) {
            double amount = getArguments().getDouble(BraveRewardsSiteBannerActivity.AMOUNT_EXTRA);
            boolean monthlyContribution = getArguments().getBoolean(
                    BraveRewardsSiteBannerActivity.IS_MONTHLY_CONTRIBUTION);
            initView(view, amount, monthlyContribution);
        }
        return view;
    }

    @SuppressLint("SetTextI18n")
    private void initView(View view, double amount, boolean monthlyContribution) {
        TextView tipAmountTextView = view.findViewById(R.id.amount_sent);
        tipAmountTextView.setText(amount + " BAT");
        TextView tipTypeDescriptionTextView = view.findViewById(R.id.your_tip_amount_sent);

        if (monthlyContribution) {
            tipTypeDescriptionTextView.setText(
                    getResources().getString(R.string.your_monthly_tip_has_been_sent));
        } else {
            tipTypeDescriptionTextView.setText(
                    getResources().getString(R.string.your_one_time_tip_has_been_sent));
        }
    }
}
