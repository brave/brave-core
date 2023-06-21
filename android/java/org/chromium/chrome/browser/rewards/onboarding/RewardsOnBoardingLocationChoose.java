/**
 * Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.rewards.onboarding;

import android.annotation.SuppressLint;
import android.content.Context;
import android.os.Build;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.PopupWindow;
import android.widget.Spinner;

import androidx.core.app.ActivityCompat;
import androidx.core.content.res.ResourcesCompat;

import org.chromium.base.BuildInfo;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.BraveRewardsNativeWorker;
import org.chromium.chrome.browser.BraveRewardsObserver;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.notifications.BraveNotificationWarningDialog;
import org.chromium.chrome.browser.notifications.BravePermissionUtils;
import org.chromium.chrome.browser.rewards.CountrySelectionSpinnerAdapter;
import org.chromium.ui.permissions.PermissionConstants;

import java.util.ArrayList;
import java.util.Locale;
import java.util.TreeMap;

public class RewardsOnBoardingLocationChoose implements BraveRewardsObserver {
    private final View mAnchorView;
    private final PopupWindow mPopupWindow;
    private ChromeTabbedActivity mActivity;
    private int mDeviceWidth;
    private static final String SUCCESS = "success";

    public RewardsOnBoardingLocationChoose(View anchorView, int deviceWidth) {
        mDeviceWidth = deviceWidth;
        mAnchorView = anchorView;
        mPopupWindow = new PopupWindow(anchorView.getContext());
        mPopupWindow.setHeight(ViewGroup.LayoutParams.WRAP_CONTENT);
        mPopupWindow.setBackgroundDrawable(ResourcesCompat.getDrawable(
                anchorView.getContext().getResources(), R.drawable.rewards_panel_background, null));

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            mPopupWindow.setElevation(20);
        }
        mActivity = BraveRewardsHelper.getChromeTabbedActivity();

        setUpViews();
    }

    BraveRewardsNativeWorker mBraveRewardsNativeWorker;
    View mPopupView;
    Button mContinueButton;

    private void setUpViews() {
        LayoutInflater inflater = (LayoutInflater) mAnchorView.getContext().getSystemService(
                Context.LAYOUT_INFLATER_SERVICE);
        mPopupView = inflater.inflate(R.layout.rewards_onboarding_location_choose, null);
        mContinueButton = mPopupView.findViewById(R.id.btn_continue);
        mPopupWindow.setWidth(mDeviceWidth);
        mPopupWindow.setContentView(mPopupView);
        mBraveRewardsNativeWorker = BraveRewardsNativeWorker.getInstance();
        mBraveRewardsNativeWorker.AddObserver(this);
        mBraveRewardsNativeWorker.getAvailableCountries();
        setDismissListener();
    }

    private void setDismissListener() {
        mPopupWindow.setOnDismissListener(new PopupWindow.OnDismissListener() {
            @Override
            public void onDismiss() {
                if (mBraveRewardsNativeWorker != null) {
                    mBraveRewardsNativeWorker.RemoveObserver(RewardsOnBoardingLocationChoose.this);
                }
            }
        });
    }

    @Override
    public void onGetAvailableCountries(String[] countries) {
        View progressBar = mPopupView.findViewById(R.id.progressBar);
        progressBar.setVisibility(View.GONE);
        updateCountryList(countries);
    }

    private void updateCountryList(String[] countries) {
        TreeMap<String, String> sortedCountryMap = new TreeMap<String, String>();
        for (String countryCode : countries) {
            sortedCountryMap.put(new Locale("", countryCode).getDisplayCountry(), countryCode);
        }

        ArrayList<String> countryList = new ArrayList<String>();
        countryList.add(mActivity.getResources().getString(R.string.select_your_country_title));
        countryList.addAll(sortedCountryMap.keySet());
        String defaultCountry = mBraveRewardsNativeWorker.getCountryCode() != null
                ? new Locale("", mBraveRewardsNativeWorker.getCountryCode()).getDisplayCountry()
                : null;
        if (defaultCountry != null && countryList.contains(defaultCountry)) {
            countryList.remove(defaultCountry);
            countryList.add(1, defaultCountry);
        }

        String[] countryArray = countryList.toArray(new String[countryList.size()]);
        Spinner countrySpinner;
        countrySpinner = mPopupView.findViewById(R.id.country_spinner);

        // if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
        //     countrySpinner = popupView.findViewById(R.id.country_spinner);
        // } else {
        //     // countrySpinner = mBraveRewardsOnboardingModalView.findViewById(
        //     //         R.id.country_spinner_low_device);
        // }
        CountrySelectionSpinnerAdapter countrySelectionSpinnerAdapter =
                new CountrySelectionSpinnerAdapter(mActivity, countryList);
        countrySpinner.setAdapter(countrySelectionSpinnerAdapter);
        countrySpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int pos, long id) {
                if (pos != 0) {
                    mContinueButton.setEnabled(true);
                } else {
                    mContinueButton.setEnabled(false);
                }
            }
            @Override
            public void onNothingSelected(AdapterView<?> arg0) {}
        });

        mContinueButton.setOnClickListener((new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (!BravePermissionUtils.hasPermission(
                            mAnchorView.getContext(), PermissionConstants.NOTIFICATION_PERMISSION)
                        || BravePermissionUtils.isBraveAdsNotificationPermissionBlocked(
                                mAnchorView.getContext())) {
                    requestNotificationPermission();
                }
                if (countrySpinner != null) {
                    mBraveRewardsNativeWorker.CreateRewardsWallet(
                            sortedCountryMap.get(countrySpinner.getSelectedItem().toString()));
                    View progressBar = mPopupView.findViewById(R.id.continue_progress_bar);
                    progressBar.setVisibility(View.VISIBLE);
                    mContinueButton.setText("");
                }
            }
        }));
    }

    @Override
    public void onCreateRewardsWallet(String result) {
        mPopupWindow.dismiss();

        if (result.equals(SUCCESS)) {
            RewardsOnBoardingAllSet panel = new RewardsOnBoardingAllSet(mAnchorView, mDeviceWidth);
            panel.showLikePopDownMenu();
        } else {
            RewardsOnBoardingError panel =
                    new RewardsOnBoardingError(mAnchorView, mDeviceWidth, result);
            panel.showLikePopDownMenu();
        }
    }

    private void requestNotificationPermission() {
        if (BravePermissionUtils.isBraveAdsNotificationPermissionBlocked(mAnchorView.getContext())
                || mActivity.shouldShowRequestPermissionRationale(
                        PermissionConstants.NOTIFICATION_PERMISSION)
                || (!BuildInfo.isAtLeastT() || !BuildInfo.targetsAtLeastT())) {
            // other than android 13 redirect to
            // setting page and for android 13 Last time don't allow selected in permission
            // dialog, then enable through setting, this done through this dialog
            showNotificationWarningDialog();
        } else {
            // 1st time request permission
            ActivityCompat.requestPermissions(
                    mActivity, new String[] {PermissionConstants.NOTIFICATION_PERMISSION}, 1);
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

    public void showLikePopDownMenu() {
        mPopupWindow.setTouchable(true);
        mPopupWindow.setFocusable(true);
        mPopupWindow.setOutsideTouchable(true);

        mPopupWindow.showAsDropDown(mAnchorView, 0, 0);
    }
}
