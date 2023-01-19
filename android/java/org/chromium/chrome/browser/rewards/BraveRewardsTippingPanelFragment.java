/**
 * Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.rewards;
;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Typeface;
import android.os.Bundle;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.TextPaint;
import android.text.method.LinkMovementMethod;
import android.text.style.ClickableSpan;
import android.text.style.ForegroundColorSpan;
import android.text.style.StyleSpan;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.TranslateAnimation;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.TextView;
import android.widget.ToggleButton;

import androidx.annotation.NonNull;
import androidx.core.content.ContextCompat;
import androidx.core.content.res.ResourcesCompat;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveRewardsBalance;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.BraveRewardsNativeWorker;
import org.chromium.chrome.browser.BraveRewardsObserver;
import org.chromium.chrome.browser.BraveRewardsSiteBannerActivity;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.customtabs.CustomTabActivity;
import org.chromium.ui.widget.Toast;

import java.math.RoundingMode;
import java.text.DecimalFormat;
import java.util.Locale;

public class BraveRewardsTippingPanelFragment extends Fragment implements BraveRewardsObserver {
    private int currentTabId_;
    private boolean monthlyContribution;

    private ToggleButton radio_tip_amount[] = new ToggleButton[3];
    private BraveRewardsNativeWorker mBraveRewardsNativeWorker;
    private LinearLayout sendDonationLayout;
    private boolean mTippingInProgress; // flag preventing multiple tipping processes
    private RadioButton monthlyRadioButton;
    private boolean notEnoughFundsShown;
    private double mBatValue;
    private double mUsdValue;
    private double[] mAmounts;
    private double[] mTipChoices;

    private final int TIP_SENT_REQUEST_CODE = 2;
    private final int FADE_OUT_DURATION = 500;

    private static final int DEFAULT_VALUE_OPTION_1 = 1;
    private static final int DEFAULT_VALUE_OPTION_2 = 5;
    private static final int DEFAULT_VALUE_OPTION_3 = 10;

    private static final double DEFAULT_AMOUNT = 0.0;

    private static final String CUSTOM_TIP_CONFIRMATION_FRAGMENT_TAG =
            "custom_tip_confirmation_fragment";
    private static final String CUSTOM_TIP_FRAGMENT_TAG = "custom_tip_fragment";

    public static BraveRewardsTippingPanelFragment newInstance(
            int tabId, boolean isMonthlyContribution, double[] amounts) {
        BraveRewardsTippingPanelFragment fragment = new BraveRewardsTippingPanelFragment();

        Bundle args = new Bundle();
        args.putInt(BraveRewardsSiteBannerActivity.TAB_ID_EXTRA, tabId);
        args.putBoolean(
                BraveRewardsSiteBannerActivity.IS_MONTHLY_CONTRIBUTION, isMonthlyContribution);
        args.putDoubleArray(BraveRewardsSiteBannerActivity.AMOUNTS_ARGS, amounts);
        fragment.setArguments(args);
        return fragment;
    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        mBatValue = DEFAULT_AMOUNT;
        mUsdValue = DEFAULT_AMOUNT;
        if (getArguments() != null) {
            currentTabId_ = getArguments().getInt(BraveRewardsSiteBannerActivity.TAB_ID_EXTRA);
            monthlyContribution = getArguments().getBoolean(
                    BraveRewardsSiteBannerActivity.IS_MONTHLY_CONTRIBUTION);
            mAmounts = getArguments().getDoubleArray(BraveRewardsSiteBannerActivity.AMOUNTS_ARGS);
        }
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.brave_rewards_tipping_panel, container, false);
        addRadioButtonListener(view);
        init(view);
        updateTermsOfServicePlaceHolder(view);
        initRadioButtons(view);
        setBalanceText(view);
        sendTipButtonClick();
        clickOtherAmounts(view);
        return view;
    }

    private void addRadioButtonListener(View view) {
        RadioGroup radioGroup =
                (RadioGroup) view.findViewById(R.id.one_time_or_monthly_radio_group);
        radioGroup.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(RadioGroup group, int checkedId) {
                RadioButton oneTimeRadioButton =
                        (RadioButton) view.findViewById(R.id.one_time_radio_button);
                RadioButton monthlyRadio =
                        (RadioButton) view.findViewById(R.id.monthly_radio_button);
                if (getParentFragmentManager().getBackStackEntryCount() != 0) {
                    ((BraveRewardsSiteBannerActivity) getActivity()).resetUpdateLayout();
                }
                if (checkedId == R.id.monthly_radio_button) {
                    oneTimeRadioButton.setTypeface(Typeface.DEFAULT, Typeface.NORMAL);
                    monthlyRadio.setTypeface(Typeface.DEFAULT_BOLD, Typeface.BOLD);
                    monthlyContribution = true;
                    monthlyLayoutText();
                } else {
                    monthlyRadio.setTypeface(Typeface.DEFAULT, Typeface.NORMAL);
                    oneTimeRadioButton.setTypeface(Typeface.DEFAULT_BOLD, Typeface.BOLD);
                    monthlyContribution = false;
                    resetSendLayoutText();
                }
            }
        });
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        if (null != mBraveRewardsNativeWorker) {
            mBraveRewardsNativeWorker.RemoveObserver(this);
        }
    }

    private void init(View view) {
        mBraveRewardsNativeWorker = BraveRewardsNativeWorker.getInstance();
        mBraveRewardsNativeWorker.AddObserver(this);

        sendDonationLayout = view.findViewById(R.id.send_donation_button);
        monthlyRadioButton = view.findViewById(R.id.monthly_radio_button);
        monthlyRadioButton.setChecked(monthlyContribution);
        notEnoughFundsShown = false;
    }

    private void clickOtherAmounts(View view) {
        TextView otherAmountsText = view.findViewById(R.id.other_amounts);
        otherAmountsText.setOnClickListener(v -> {
            double amount = selectedAmount();

            FragmentManager fragmentManager = getParentFragmentManager();
            fragmentManager.beginTransaction()
                    .replace(R.id.conversionFragmentContainer,
                            BraveRewardsCustomTipFragment.newInstance(amount),
                            CUSTOM_TIP_FRAGMENT_TAG)
                    .addToBackStack(null)
                    .commit();
        });
    }

    private void initRadioButtons(View view) {
        mTipChoices = mBraveRewardsNativeWorker.GetTipChoices();
        // when native not giving tip choices initialize with default values
        if (mTipChoices.length < 3) {
            mTipChoices = new double[3];
            mTipChoices[0] = DEFAULT_VALUE_OPTION_1;
            mTipChoices[1] = DEFAULT_VALUE_OPTION_2;
            mTipChoices[2] = DEFAULT_VALUE_OPTION_3;
        }
        double rate = mBraveRewardsNativeWorker.GetWalletRate();

        int recurrentAmount = (int) mBraveRewardsNativeWorker.GetPublisherRecurrentDonationAmount(
                mBraveRewardsNativeWorker.GetPublisherId(currentTabId_));

        // bind tip amount custom radio buttons
        radio_tip_amount[0] = view.findViewById(R.id.bat_option1);
        radio_tip_amount[1] = view.findViewById(R.id.bat_option2);
        radio_tip_amount[2] = view.findViewById(R.id.bat_option3);

        for (int i = 0; i < 3; i++) {
            radio_tip_amount[i].setTextOff(mTipChoices[i] + " " + BraveRewardsHelper.BAT_TEXT);
            radio_tip_amount[i].setTextOn(mTipChoices[i] + " " + BraveRewardsHelper.BAT_TEXT);
            radio_tip_amount[i].setChecked(false);
        }

        for (ToggleButton tb : radio_tip_amount) {
            tb.setOnClickListener(radio_clicker);
        }

        double usdValue = mTipChoices[0] * rate;
        double fiveBat = mTipChoices[1] * rate;
        double tenBat = mTipChoices[2] * rate;
        String oneBatRate = String.format(Locale.getDefault(), "%.2f", usdValue) + " USD";
        String fiveBatRate = String.format(Locale.getDefault(), "%.2f", fiveBat) + " USD";
        String tenBatRate = String.format(Locale.getDefault(), "%.2f", tenBat) + " USD";
        ((TextView) view.findViewById(R.id.amount1)).setText(oneBatRate);
        ((TextView) view.findViewById(R.id.amount2)).setText(fiveBatRate);
        ((TextView) view.findViewById(R.id.amount3)).setText(tenBatRate);
    }

    private View.OnClickListener radio_clicker = view -> {
        ToggleButton tb_pressed = (ToggleButton) view;
        if (!tb_pressed.isChecked()) {
            tb_pressed.setChecked(true);
            return;
        }

        int id = view.getId();
        for (ToggleButton tb : radio_tip_amount) {
            if (tb.getId() == id) {
                continue;
            }
            tb.setChecked(false);
        }
    };

    private double selectedAmount() {
        double amount = DEFAULT_AMOUNT;
        for (ToggleButton tb : radio_tip_amount) {
            if (tb.isChecked()) {
                int id = tb.getId();
                if (id == R.id.bat_option1) {
                    amount = mTipChoices[0];
                } else if (id == R.id.bat_option2) {
                    amount = mTipChoices[1];
                } else if (id == R.id.bat_option3) {
                    amount = mTipChoices[2];
                }

                break;
            }
        }
        // Selected amount from custom tip
        BraveRewardsCustomTipConfirmationFragment customTipConfirmationFragment =
                (BraveRewardsCustomTipConfirmationFragment) getParentFragmentManager()
                        .findFragmentByTag(CUSTOM_TIP_CONFIRMATION_FRAGMENT_TAG);
        if (customTipConfirmationFragment != null && customTipConfirmationFragment.isVisible()) {
            amount = mBatValue;
        }

        return amount;
    }

    private boolean isCustomTipFragmentVisibile() {
        BraveRewardsCustomTipFragment customTipFragment =
                (BraveRewardsCustomTipFragment) getParentFragmentManager().findFragmentByTag(
                        CUSTOM_TIP_FRAGMENT_TAG);
        return customTipFragment != null && customTipFragment.isVisible();
    }

    private void showCustomTipConfirmationFrament() {
        if (getParentFragmentManager().getBackStackEntryCount() != 0) {
            getParentFragmentManager().popBackStack();
        }
        resetSendLayoutText();

        FragmentManager fragmentManager = getParentFragmentManager();
        fragmentManager.beginTransaction()
                .replace(R.id.conversionFragmentContainer,
                        BraveRewardsCustomTipConfirmationFragment.newInstance(
                                monthlyContribution, mBatValue, mUsdValue),
                        CUSTOM_TIP_CONFIRMATION_FRAGMENT_TAG)
                .addToBackStack(null)
                .commit();
    }

    private void donateAndShowConfirmationScreen(double amount) {
        if (mTippingInProgress) {
            return;
        }
        mTippingInProgress = true;
        boolean monthly_bool = monthlyRadioButton.isChecked();

        mBraveRewardsNativeWorker.Donate(
                mBraveRewardsNativeWorker.GetPublisherId(currentTabId_), amount, monthly_bool);

        replaceTipConfirmationFragment(amount, monthly_bool);
    }

    private void showNotEnoughFunds() {
        ImageView imageView = sendDonationLayout.findViewById(R.id.send_tip_image);
        TextView sendDonationText = sendDonationLayout.findViewById(R.id.send_donation_text);

        // Not funds case toggle the text and image
        if (!notEnoughFundsShown) {
            notEnoughFundsShown = true;
            imageView.setImageResource(R.drawable.icn_frowning_face);
            sendDonationText.setText(
                    String.format(getResources().getString(R.string.brave_ui_not_enough_tokens),
                            getResources().getString(R.string.token)));
        } else {
            notEnoughFundsShown = false;
            imageView.setImageResource(R.drawable.ic_send_tip);
            sendDonationText.setText(getResources().getString(R.string.send_tip));
        }
        int fromY = sendDonationLayout.getHeight();

        TranslateAnimation animate = new TranslateAnimation(0, 0, fromY, 0);
        animate.setDuration(FADE_OUT_DURATION);

        sendDonationLayout.startAnimation(animate);
    }

    private double getBalance() {
        double balance = .0;
        BraveRewardsBalance rewards_balance = mBraveRewardsNativeWorker.GetWalletBalance();
        if (rewards_balance != null) {
            balance = rewards_balance.getTotal();
        }
        return balance;
    }

    private void sendTipButtonClick() {
        sendDonationLayout.setOnClickListener(v -> {
            double balance = getBalance();
            double amount = selectedAmount();
            boolean enough_funds = ((balance - amount) >= DEFAULT_AMOUNT);

            // When custom tip with enough funds is there then continue button will show
            // click on continue button will show CustomTipConfirmation UI
            if (isCustomTipFragmentVisibile()) {
                enough_funds = ((balance - mBatValue) >= DEFAULT_AMOUNT) && mBatValue >= 0.25;
                if (enough_funds) {
                    showCustomTipConfirmationFrament();
                } // else nothing
            }
            // proceed to tipping
            else if (enough_funds) {
                if (amount == DEFAULT_AMOUNT) {
                    Toast.makeText(getActivity(), R.string.rewards_tipping_select_amount,
                                 Toast.LENGTH_SHORT)
                            .show();
                    return;
                }
                donateAndShowConfirmationScreen(amount);
            }
            // if not enough funds
            else {
                showNotEnoughFunds();
            }
        });
    }

    public void replaceTipConfirmationFragment(double amount, boolean isMonthly) {
        BraveRewardsTipConfirmationListener confirmation =
                (BraveRewardsTipConfirmationListener) getActivity();
        confirmation.onTipConfirmation(amount, isMonthly);
    }

    private void updateTermsOfServicePlaceHolder(View view) {
        Resources res = getResources();
        TextView proceedTextView = view.findViewById(R.id.proceed_terms_of_service);
        proceedTextView.setMovementMethod(LinkMovementMethod.getInstance());
        String termsOfServiceText = String.format(res.getString(R.string.brave_rewards_tos_text),
                res.getString(R.string.terms_of_service), res.getString(R.string.privacy_policy));

        SpannableString spannableString = stringToSpannableString(termsOfServiceText);
        proceedTextView.setText(spannableString);
    }

    private SpannableString stringToSpannableString(String text) {
        Spanned textSpanned = BraveRewardsHelper.spannedFromHtmlString(text);
        SpannableString textSpannableString = new SpannableString(textSpanned.toString());
        setSpan(text, textSpannableString, R.string.terms_of_service,
                termsOfServiceClickableSpan); // terms of service
        setSpan(text, textSpannableString, R.string.privacy_policy,
                privacyPolicyClickableSpan); // privacy policy
        return textSpannableString;
    }

    private void setSpan(
            String text, SpannableString tosTextSS, int stringId, ClickableSpan clickableSpan) {
        String spanString = getResources().getString(stringId);
        int spanLength = spanString.length();
        int index = text.indexOf(spanString);

        StyleSpan boldSpan = new StyleSpan(Typeface.BOLD);
        tosTextSS.setSpan(boldSpan, index, index + spanLength, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        tosTextSS.setSpan(
                clickableSpan, index, index + spanLength, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        Typeface typeface = ResourcesCompat.getFont(getActivity(), R.font.poppins_500);
        tosTextSS.setSpan(new StyleSpan(typeface.getStyle()), index, index + spanLength,
                Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        tosTextSS.setSpan(new ForegroundColorSpan(
                                  getResources().getColor(R.color.brave_rewards_modal_theme_color)),
                index, index + spanLength, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
    }

    private ClickableSpan termsOfServiceClickableSpan = new ClickableSpan() {
        @Override
        public void onClick(@NonNull View textView) {
            CustomTabActivity.showInfoPage(getActivity(), BraveActivity.BRAVE_TERMS_PAGE);
        }
        @Override
        public void updateDrawState(@NonNull TextPaint ds) {
            super.updateDrawState(ds);
            ds.setUnderlineText(false);
        }
    };

    private ClickableSpan privacyPolicyClickableSpan = new ClickableSpan() {
        @Override
        public void onClick(@NonNull View textView) {
            CustomTabActivity.showInfoPage(getActivity(), BraveActivity.BRAVE_PRIVACY_POLICY);
        }
        @Override
        public void updateDrawState(@NonNull TextPaint ds) {
            super.updateDrawState(ds);
            ds.setUnderlineText(false);
        }
    };

    void setBalanceText(View view) {
        double balance = DEFAULT_AMOUNT;
        BraveRewardsBalance rewards_balance = mBraveRewardsNativeWorker.GetWalletBalance();
        if (rewards_balance != null) {
            balance = rewards_balance.getTotal();
        }

        DecimalFormat df = new DecimalFormat("#.###");
        df.setRoundingMode(RoundingMode.FLOOR);
        df.setMinimumFractionDigits(3);
        String walletAmount = df.format(balance) + " " + BraveRewardsHelper.BAT_TEXT;

        ((TextView) view.findViewById(R.id.wallet_amount_text)).setText(walletAmount);
    }

    public void updateAmount(double batValue, double usdValue) {
        mBatValue = batValue;
        mUsdValue = usdValue;
        double balance = DEFAULT_AMOUNT;
        BraveRewardsBalance rewards_balance = mBraveRewardsNativeWorker.GetWalletBalance();
        if (rewards_balance != null) {
            balance = rewards_balance.getTotal();
        }
        if (batValue >= 0.25 && batValue <= balance) {
            continueButtonLayout();
        } else if (batValue >= balance) {
            notEnoughFundLayout();
        } else if (batValue < 0.25) {
            minimumTipAmountLayout();
        } else {
            resetSendLayoutText();
        }
    }

    private void minimumTipAmountLayout() {
        ImageView imageView = sendDonationLayout.findViewById(R.id.send_tip_image);
        sendDonationLayout.setBackgroundColor(ContextCompat.getColor(
                getContext(), R.color.rewards_send_tip_minimum_balance_background));
        TextView sendDonationText = sendDonationLayout.findViewById(R.id.send_donation_text);
        sendDonationText.setText(getResources().getString(R.string.minimum_tip_amount));
        sendDonationText.setTextColor(ContextCompat.getColor(getContext(), android.R.color.black));
        imageView.setImageResource(R.drawable.icn_frowning_face);
        imageView.setColorFilter(ContextCompat.getColor(
                getContext(), R.color.rewards_send_tip_icon_minimum_balance_tint));
        imageView.setVisibility(View.VISIBLE);
    }

    private void notEnoughFundLayout() {
        ImageView imageView = sendDonationLayout.findViewById(R.id.send_tip_image);
        sendDonationLayout.setBackgroundColor(ContextCompat.getColor(
                getContext(), R.color.rewards_send_tip_not_enough_fund_background));
        TextView sendDonationText = sendDonationLayout.findViewById(R.id.send_donation_text);
        sendDonationText.setText(getResources().getString(R.string.not_enough_fund));
        sendDonationText.setTextColor(ContextCompat.getColor(getContext(), android.R.color.white));
        imageView.setImageResource(R.drawable.icn_frowning_face);
        imageView.setColorFilter(ContextCompat.getColor(
                getContext(), R.color.rewards_send_tip_icon_not_enough_fund_tint));
        imageView.setVisibility(View.VISIBLE);
    }

    private void continueButtonLayout() {
        ImageView imageView = sendDonationLayout.findViewById(R.id.send_tip_image);
        sendDonationLayout.setBackgroundColor(
                ContextCompat.getColor(getContext(), R.color.rewards_send_tip_background));
        TextView sendDonationText = sendDonationLayout.findViewById(R.id.send_donation_text);
        sendDonationText.setText(getResources().getString(R.string.continue_tip));
        sendDonationText.setTextColor(ContextCompat.getColor(getContext(), android.R.color.white));
        imageView.setImageResource(R.drawable.ic_send_tip);
        imageView.setColorFilter(
                ContextCompat.getColor(getContext(), R.color.rewards_send_tip_icon_tint));
        imageView.setVisibility(View.GONE);
    }

    public void monthlyLayoutText() {
        ImageView imageView = sendDonationLayout.findViewById(R.id.send_tip_image);
        sendDonationLayout.setBackgroundColor(
                ContextCompat.getColor(getContext(), R.color.rewards_send_tip_background));
        TextView sendDonationText = sendDonationLayout.findViewById(R.id.send_donation_text);
        sendDonationText.setText(getResources().getString(R.string.set_monthly_tip));
        sendDonationText.setTextColor(ContextCompat.getColor(getContext(), android.R.color.white));
        imageView.setImageResource(R.drawable.ic_calendar_icon);
        imageView.setVisibility(View.VISIBLE);
    }

    public void resetSendLayoutText() {
        ImageView imageView = sendDonationLayout.findViewById(R.id.send_tip_image);
        sendDonationLayout.setBackgroundColor(
                ContextCompat.getColor(getContext(), R.color.rewards_send_tip_background));
        TextView sendDonationText = sendDonationLayout.findViewById(R.id.send_donation_text);
        sendDonationText.setText(getResources().getString(R.string.send_tip));
        sendDonationText.setTextColor(ContextCompat.getColor(getContext(), android.R.color.white));
        imageView.setImageResource(R.drawable.ic_send_tip);
        imageView.setVisibility(View.VISIBLE);
    }
}
