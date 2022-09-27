/**
 * Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.rewards;

import android.annotation.SuppressLint;
import android.os.Bundle;
import android.text.Editable;
import android.text.InputFilter;
import android.text.Spanned;
import android.text.TextWatcher;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.fragment.app.Fragment;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.BraveRewardsSiteBannerActivity;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class BraveRewardsCustomTipConfirmationFragment extends Fragment {
    private static final String CONFIRM_BAT_VALUE = "confirm_bat_value";
    private static final String CONFIRM_USD_VALUE = "confirm_usd_value";
    private static final String IS_MONTHLY_CONTRIBUTION = "is_monthly_contribution";

    public static BraveRewardsCustomTipConfirmationFragment newInstance(
            boolean isMonthlyContribution, double batValue, double usdValue) {
        BraveRewardsCustomTipConfirmationFragment fragment =
                new BraveRewardsCustomTipConfirmationFragment();
        Bundle args = new Bundle();
        args.putDouble(CONFIRM_BAT_VALUE, batValue);
        args.putDouble(CONFIRM_USD_VALUE, usdValue);
        args.putBoolean(IS_MONTHLY_CONTRIBUTION, isMonthlyContribution);
        fragment.setArguments(args);
        return fragment;
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View view = inflater.inflate(
                R.layout.brave_rewards_custom_tip_confirmation_fragment, container, false);
        updateValues(view);
        cancelButtonClick(view);
        return view;
    }

    @SuppressLint("SetTextI18n")
    private void updateValues(View view) {
        TextView batValueText = view.findViewById(R.id.batValueText);
        TextView usdValueText = view.findViewById(R.id.usdValueText);
        TextView aboutToTipText = view.findViewById(R.id.about_to_tip);
        if (getArguments() != null) {
            double batValue = getArguments().getDouble(CONFIRM_BAT_VALUE);
            double usdValue = getArguments().getDouble(CONFIRM_USD_VALUE);
            Boolean isMonthlyContribution = getArguments().getBoolean(IS_MONTHLY_CONTRIBUTION);

            if (isMonthlyContribution) {
                aboutToTipText.setText(R.string.your_monthly_support);
            } else {
                aboutToTipText.setText(R.string.about_to_tip);
            }

            batValueText.setText(batValue + " " + BraveRewardsHelper.BAT_TEXT);
            usdValueText.setText(usdValue + " " + BraveRewardsHelper.USD_TEXT);
        }
    }

    private void cancelButtonClick(View view) {
        ImageView customTipCancel = view.findViewById(R.id.custom_tip_cancel);
        customTipCancel.setOnClickListener((v) -> {
            if (getParentFragmentManager().getBackStackEntryCount() > 0) {
                getParentFragmentManager().popBackStack();
            }
        });
    }
}
