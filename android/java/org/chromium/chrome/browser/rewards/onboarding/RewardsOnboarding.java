/**
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.rewards.onboarding;

import android.Manifest;
import android.annotation.SuppressLint;
import android.content.Context;
import android.os.Build;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.method.LinkMovementMethod;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.PopupWindow;
import android.widget.Spinner;
import android.widget.TextView;

import androidx.core.app.ActivityCompat;
import androidx.core.content.res.ResourcesCompat;

import org.chromium.base.BuildInfo;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.BraveRewardsNativeWorker;
import org.chromium.chrome.browser.BraveRewardsObserver;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.customtabs.CustomTabActivity;
import org.chromium.chrome.browser.notifications.BraveNotificationWarningDialog;
import org.chromium.chrome.browser.notifications.BravePermissionUtils;
import org.chromium.chrome.browser.rewards.BraveRewardsPanel;
import org.chromium.chrome.browser.util.BraveTouchUtils;
import org.chromium.chrome.browser.util.TabUtils;
import org.chromium.ui.text.ChromeClickableSpan;

import java.text.Collator;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Locale;
import java.util.TreeMap;

/**
 * This class is used to show rewards onBoarding UI
 **/
public class RewardsOnboarding implements BraveRewardsObserver {
    private final View mAnchorView;
    private final PopupWindow mPopupWindow;
    private View mPopupView;

    private ViewGroup mMainLayout;
    private ViewGroup mLocationChooseLayout;
    private ViewGroup mAllSetLayout;
    private ViewGroup mErrorLayout;

    private Spinner mCountrySpinner;
    private TextView mContinueButton;

    private BraveRewardsNativeWorker mBraveRewardsNativeWorker;

    private ChromeTabbedActivity mActivity;

    private static final String SUCCESS = "success";

    public RewardsOnboarding(View anchorView, int deviceWidth) {
        mAnchorView = anchorView;
        mPopupWindow = new PopupWindow(anchorView.getContext());
        mPopupWindow.setHeight(ViewGroup.LayoutParams.WRAP_CONTENT);
        mPopupWindow.setBackgroundDrawable(ResourcesCompat.getDrawable(
                anchorView.getContext().getResources(), R.drawable.rewards_panel_background, null));

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            mPopupWindow.setElevation(20);
        }

        mActivity = BraveRewardsHelper.getChromeTabbedActivity();

        setUpViews(deviceWidth);
    }

    private void setUpViews(int deviceWidth) {
        LayoutInflater inflater = (LayoutInflater) mAnchorView.getContext().getSystemService(
                Context.LAYOUT_INFLATER_SERVICE);
        mPopupView = inflater.inflate(R.layout.rewards_onboarding, null);

        // Main layout views
        mMainLayout = mPopupView.findViewById(R.id.rewards_onboarding_layout_id);
        View startUsingButton = mMainLayout.findViewById(R.id.start_using_rewards_button);
        startUsingButton.setOnClickListener(v -> {
            mBraveRewardsNativeWorker.getAvailableCountries();
            mLocationChooseLayout.setVisibility(View.VISIBLE);
            mMainLayout.setVisibility(View.GONE);
        });
        View howDoseItWorkMainButton = mMainLayout.findViewById(R.id.how_does_it_work_main);
        howDoseItWorkMainButton.setOnClickListener(v -> { showRewardsTour(); });

        mCountrySpinner = mPopupView.findViewById(R.id.country_spinner);
        BraveTouchUtils.ensureMinTouchTarget(mCountrySpinner);

        String termsOfServiceText = String.format(
                mPopupView.getContext().getString(R.string.brave_rewards_onboarding_tos_text),
                mPopupView.getContext().getString(R.string.terms_of_service),
                mPopupView.getContext().getString(R.string.privacy_policy));
        TextView tosText = mMainLayout.findViewById(R.id.tos_text);
        tosText.setMovementMethod(LinkMovementMethod.getInstance());
        tosText.setText(BraveRewardsHelper.tosSpannableString(
                termsOfServiceText, R.color.brave_rewards_modal_theme_color));

        // Location choose layout views
        mLocationChooseLayout =
                mPopupView.findViewById(R.id.rewards_onboarding_location_choose_layout_id);
        mContinueButton = mLocationChooseLayout.findViewById(R.id.btn_continue);

        // All set layout views
        mAllSetLayout = mPopupView.findViewById(R.id.rewards_onboarding_all_set_layout_id);
        View doneButton = mAllSetLayout.findViewById(R.id.all_set_done_button);
        doneButton.setOnClickListener(v -> {
            mPopupWindow.dismiss();
            TabUtils.openUrlInNewTab(false, BraveRewardsPanel.REWARDS_TOUR_URL);
        });
        View howDoseItWorkAllSetButton = mAllSetLayout.findViewById(R.id.how_does_it_work_all_set);
        howDoseItWorkAllSetButton.setOnClickListener(v -> { showRewardsTour(); });

        // Error layout views
        mErrorLayout = mPopupView.findViewById(R.id.rewards_onboarding_error_layout_id);
        View responseActionButton = mErrorLayout.findViewById(R.id.response_action_btn);
        responseActionButton.setOnClickListener(v -> {
            mBraveRewardsNativeWorker.getAvailableCountries();
            mLocationChooseLayout.setVisibility(View.VISIBLE);
            mErrorLayout.setVisibility(View.GONE);
        });
        View responseCloseButton = mErrorLayout.findViewById(R.id.response_modal_close);
        responseCloseButton.setOnClickListener(v -> { mPopupWindow.dismiss(); });

        mPopupWindow.setOnDismissListener(
                new PopupWindow.OnDismissListener() {
                    @Override
                    public void onDismiss() {
                        if (mBraveRewardsNativeWorker != null) {
                            mBraveRewardsNativeWorker.removeObserver(RewardsOnboarding.this);
                        }
                    }
                });

        mPopupWindow.setWidth(deviceWidth);
        mPopupWindow.setContentView(mPopupView);

        mBraveRewardsNativeWorker = BraveRewardsNativeWorker.getInstance();
        mBraveRewardsNativeWorker.addObserver(this);
    }

    public void showLikePopDownMenu() {
        mPopupWindow.setTouchable(true);
        mPopupWindow.setFocusable(true);
        mPopupWindow.setOutsideTouchable(true);
        mPopupWindow.showAsDropDown(mAnchorView, 0, 0);
    }

    private void showRewardsTour() {
        mPopupWindow.dismiss();
        TabUtils.openUrlInNewTab(false, BraveRewardsPanel.REWARDS_TOUR_URL);
    }

    @Override
    public void onGetAvailableCountries(String[] countries) {
        View progressBar = mPopupView.findViewById(R.id.progressBar);
        progressBar.setVisibility(View.GONE);
        updateCountryList(countries);
    }

    @SuppressLint("ClickableViewAccessibility")
    private void updateCountryList(String[] countries) {
        shouldShowContinueProgress(false);
        mContinueButton.setText(mActivity.getResources().getString(R.string.continue_text));
        String defaultCountry = mBraveRewardsNativeWorker.getCountryCode() != null
                ? new Locale("", mBraveRewardsNativeWorker.getCountryCode()).getDisplayCountry()
                : null;

        TreeMap<String, String> sortedCountryMap = new TreeMap<String, String>();
        ArrayList<String> list = new ArrayList<>();
        for (String countryCode : countries) {
            sortedCountryMap.put(new Locale("", countryCode).getDisplayCountry(), countryCode);
            list.add(new Locale("", countryCode).getDisplayCountry());
        }
        Collections.sort(list, Collator.getInstance());
        ArrayList<String> countryList = new ArrayList<>();
        countryList.add(mActivity.getResources().getString(R.string.select_your_country_title));
        countryList.addAll(list);

        String[] countryArray = countryList.toArray(new String[countryList.size()]);

        CountrySelectionSpinnerAdapter countrySelectionSpinnerAdapter =
                new CountrySelectionSpinnerAdapter(mActivity, countryArray);
        countrySelectionSpinnerAdapter.setDropDownViewResource(
                android.R.layout.simple_spinner_dropdown_item);
        mCountrySpinner.setAdapter(countrySelectionSpinnerAdapter);
        mCountrySpinner.setOnTouchListener((view, event) -> {
            if (event.getAction() == MotionEvent.ACTION_UP) {
                mCountrySpinner.setSelection(
                        countrySelectionSpinnerAdapter.getPosition(defaultCountry));
            }
            return false;
        });
        mCountrySpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int pos, long id) {
                mContinueButton.setEnabled(pos != 0);
            }
            @Override
            public void onNothingSelected(AdapterView<?> arg0) {}
        });

        mContinueButton.setOnClickListener(
                new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        if (!BravePermissionUtils.hasPermission(
                                        mAnchorView.getContext(),
                                        Manifest.permission.POST_NOTIFICATIONS)
                                || BravePermissionUtils.isBraveAdsNotificationPermissionBlocked(
                                        mAnchorView.getContext())) {
                            requestNotificationPermission();
                        }
                        if (mCountrySpinner != null) {
                            mBraveRewardsNativeWorker.createRewardsWallet(
                                    sortedCountryMap.get(
                                            mCountrySpinner.getSelectedItem().toString()));
                            shouldShowContinueProgress(true);
                            mContinueButton.setText("");
                        }
                    }
                });
    }

    private void shouldShowContinueProgress(boolean shouldShow) {
        View progressBar = mPopupView.findViewById(R.id.continue_progress_bar);
        progressBar.setVisibility(shouldShow ? View.VISIBLE : View.GONE);
    }

    @Override
    public void onCreateRewardsWallet(String result) {
        mLocationChooseLayout.setVisibility(View.GONE);
        if (result.equals(SUCCESS)) {
            mAllSetLayout.setVisibility(View.VISIBLE);
        } else {
            mErrorLayout.setVisibility(View.VISIBLE);
            updateErrorModalDescription(result);
        }
    }

    private void updateErrorModalDescription(String errorMessage) {
        TextView responseModalText =
                mPopupView.findViewById(R.id.rewards_onboarding_error_description);
        TextView responseRewardsBtn = mPopupView.findViewById(R.id.response_action_btn);
        TextView responseModalTitle = mPopupView.findViewById(R.id.rewards_onboarding_error_title);
        TextView responseErrorText = mPopupView.findViewById(R.id.response_error_text);

        String actionText = mPopupView.getContext().getString(R.string.retry_text);
        if (errorMessage.equals(BraveRewardsPanel.WALLET_GENERATION_DISABLED_ERROR)) {
            String title =
                    mPopupView
                            .getContext()
                            .getString(R.string.wallet_generation_disabled_error_title);
            String text =
                    String.format(
                            mPopupView
                                    .getContext()
                                    .getString(R.string.wallet_generation_disabled_error_text),
                            mPopupView.getContext().getResources().getString(R.string.learn_more));
            SpannableString spannableWithLearnMore =
                    learnMoreSpannableString(mPopupView.getContext(), text);
            responseModalText.setMovementMethod(LinkMovementMethod.getInstance());
            responseModalText.setText(spannableWithLearnMore);
            responseModalTitle.setText(title);
            actionText = mPopupView.getContext().getString(R.string.close_text);
        }
        responseRewardsBtn.setText(actionText);
        responseErrorText.setText(errorMessage);
    }

    private SpannableString learnMoreSpannableString(Context context, String text) {
        Spanned textToAgree = BraveRewardsHelper.spannedFromHtmlString(text);

        SpannableString ss = new SpannableString(textToAgree.toString());

        ChromeClickableSpan clickableSpan =
                new ChromeClickableSpan(
                        context,
                        R.color.brave_rewards_modal_theme_color,
                        (textView) -> {
                            CustomTabActivity.showInfoPage(
                                    context, BraveRewardsPanel.NEW_SIGNUP_DISABLED_URL);
                        });
        int learnMoreIndex = text.indexOf(context.getResources().getString(R.string.learn_more));

        ss.setSpan(clickableSpan, learnMoreIndex,
                learnMoreIndex + context.getResources().getString(R.string.learn_more).length(),
                Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        return ss;
    }

    private void requestNotificationPermission() {
        if (BravePermissionUtils.isBraveAdsNotificationPermissionBlocked(mAnchorView.getContext())
                || mActivity.shouldShowRequestPermissionRationale(
                        Manifest.permission.POST_NOTIFICATIONS)
                || (Build.VERSION.SDK_INT < Build.VERSION_CODES.TIRAMISU
                        || !BuildInfo.targetsAtLeastT())) {
            // other than android 13 redirect to
            // setting page and for android 13 Last time don't allow selected in permission
            // dialog, then enable through setting, this done through this dialog
            showNotificationWarningDialog();
        } else {
            // 1st time request permission
            ActivityCompat.requestPermissions(
                    mActivity, new String[] {Manifest.permission.POST_NOTIFICATIONS}, 1);
        }
    }

    private void showNotificationWarningDialog() {
        BraveNotificationWarningDialog notificationWarningDialog =
                BraveNotificationWarningDialog.newInstance(
                        BraveNotificationWarningDialog.FROM_LAUNCHED_BRAVE_PANEL);
        notificationWarningDialog.setCancelable(false);
        notificationWarningDialog.show(mActivity.getSupportFragmentManager(),
                BraveNotificationWarningDialog.NOTIFICATION_WARNING_DIALOG_TAG);
    }
}
