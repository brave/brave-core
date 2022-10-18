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
import android.text.TextUtils;
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
import org.chromium.chrome.browser.BraveRewardsNativeWorker;
import org.chromium.chrome.browser.BraveRewardsSiteBannerActivity;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class BraveRewardsCustomTipFragment extends Fragment {
    public static final int MAX_BAT_VALUE = 100;
    public static final double AMOUNT_STEP_BY = 0.25;

    private static final String SELECTED_AMOUNT = "selected_amount";

    private boolean isBatCurrencyMode = true;
    private double exchangeRate;
    private EditText currencyOneEditText;
    private TextView currencyTwoTextView;
    private double mSelectedAmount;

    public static BraveRewardsCustomTipFragment newInstance(double selectedAmount) {
        BraveRewardsCustomTipFragment fragment = new BraveRewardsCustomTipFragment();

        Bundle args = new Bundle();
        args.putDouble(SELECTED_AMOUNT, selectedAmount);
        fragment.setArguments(args);
        return fragment;
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.brave_rewards_custom_tip_fragment, container, false);
        updateText(view);
        backButtonClick(view);
        exchangeButtonClick(view);
        return view;
    }

    private void exchangeButtonClick(View view) {
        TextView currencyOneSubTextView = view.findViewById(R.id.currencyOneSubTextView);
        TextView currencyTwoSubTextView = view.findViewById(R.id.currencyTwoSubTextView);
        TextView exampleTextView = view.findViewById(R.id.sampleText);

        ImageView exchangeButton = view.findViewById(R.id.exchangeButton);
        exchangeButton.setOnClickListener(v -> {
            isBatCurrencyMode = !isBatCurrencyMode; // toggle
            String currency1 = currencyOneEditText.getText().toString();
            String currency2 = currencyTwoTextView.getText().toString();

            currencyTwoTextView.setText(currency1);
            currencyOneEditText.setText(currency2);

            currencyOneEditText.removeTextChangedListener(textChangeListener);
            if (isBatCurrencyMode) {
                currencyOneSubTextView.setText(R.string.bat);
                currencyTwoSubTextView.setText(R.string.usd);
                exampleTextView.setVisibility(View.VISIBLE);
            } else {
                currencyOneSubTextView.setText(R.string.usd);
                currencyTwoSubTextView.setText(R.string.bat);
                exampleTextView.setVisibility(View.INVISIBLE);
            }
            currencyOneEditText.addTextChangedListener(textChangeListener);
        });
    }

    private void backButtonClick(View view) {
        ImageView backButton = view.findViewById(R.id.backButton);
        backButton.setOnClickListener(v -> {
            if (getParentFragmentManager().getBackStackEntryCount() > 0) {
                ((BraveRewardsSiteBannerActivity) getActivity()).resetUpdateLayout();
            }
        });
    }

    @SuppressLint("SetTextI18n")
    private void updateText(View view) {
        exchangeRate = BraveRewardsNativeWorker.getInstance().GetWalletRate();
        currencyOneEditText = view.findViewById(R.id.currencyOneEditText);
        currencyTwoTextView = view.findViewById(R.id.currencyTwoTextView);
        currencyOneEditText.setFilters(new InputFilter[] {new DecimalDigitsInputFilter(5, 2)});
        currencyOneEditText.addTextChangedListener(textChangeListener);
        mSelectedAmount = getArguments().getDouble(SELECTED_AMOUNT);
        currencyOneEditText.setText(String.valueOf(mSelectedAmount)); // Default value
        ((BraveRewardsSiteBannerActivity) getActivity())
                .onAmountChange(mSelectedAmount, roundExchangeUp(mSelectedAmount * exchangeRate));
    }

    private TextWatcher textChangeListener = new TextWatcher() {
        @Override
        public void beforeTextChanged(CharSequence s, int start, int count, int after) {}

        @SuppressLint("SetTextI18n")
        @Override
        public void onTextChanged(CharSequence s, int start, int before, int count) {
            if (TextUtils.isEmpty(s)) s = "0";
            Double batValue = getBatValue(s.toString());
            Double usdValue = exchangeRate * batValue;

            ((BraveRewardsSiteBannerActivity) getActivity())
                    .onAmountChange(batValue, roundExchangeUp(usdValue));
            if (isBatCurrencyMode) {
                currencyTwoTextView.setText(String.valueOf(roundExchangeUp(usdValue)));
            } else {
                currencyTwoTextView.setText(String.valueOf(batValue));
            }
        }

        @Override
        public void afterTextChanged(Editable s) {}
    };

    class DecimalDigitsInputFilter implements InputFilter {
        private Pattern mPattern;

        DecimalDigitsInputFilter(int digitsBeforeZero, int digitsAfterZero) {
            mPattern = Pattern.compile("[0-9]{0," + (digitsBeforeZero - 1) + "}+((\\.[0-9]{0,"
                    + (digitsAfterZero - 1) + "})?)||(\\.)?");
        }

        @Override
        public CharSequence filter(
                CharSequence source, int start, int end, Spanned dest, int dstart, int dend) {
            Matcher matcher = mPattern.matcher(dest);
            if (!matcher.matches()) return "";
            return null;
        }
    }

    /**
     *
     * @param inputValue : editText string passed here
     * @return
     * convert to bat value if current editText is USD
     * It will return multiple 0.25
     * And also return which is lower between Max and value
     * decimal points always roundedOff to floor value.
     */
    private double getBatValue(String inputValue) {
        double rawValue = 0;
        try {
            rawValue = Double.parseDouble(inputValue);
        } catch (NumberFormatException e) {
        }

        if (!isBatCurrencyMode) {
            // from USD to BAT
            rawValue = rawValue / exchangeRate;
        }

        // Round value with 2 decimal
        rawValue = Math.floor(rawValue / AMOUNT_STEP_BY) * AMOUNT_STEP_BY;

        // There is a limit
        return Math.min(MAX_BAT_VALUE, rawValue);
    }

    private double roundExchangeUp(double batValue) {
        return Math.ceil(batValue * 100) / 100;
    }
}
