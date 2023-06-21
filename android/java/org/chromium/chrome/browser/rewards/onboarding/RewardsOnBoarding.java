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
import android.widget.PopupWindow;

import androidx.core.content.res.ResourcesCompat;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.rewards.BraveRewardsPanel;
import org.chromium.chrome.browser.util.TabUtils;

/**
 * This class is used to show rewards onBoarding UI
 **/
public class RewardsOnBoarding {
    private final View mAnchorView;
    private final PopupWindow mPopupWindow;

    public RewardsOnBoarding(View anchorView, int deviceWidth) {
        mAnchorView = anchorView;
        mPopupWindow = new PopupWindow(anchorView.getContext());
        mPopupWindow.setHeight(ViewGroup.LayoutParams.WRAP_CONTENT);
        mPopupWindow.setBackgroundDrawable(ResourcesCompat.getDrawable(
                anchorView.getContext().getResources(), R.drawable.rewards_panel_background, null));

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            mPopupWindow.setElevation(20);
        }

        setUpViews(deviceWidth);
    }

    private void setUpViews(int deviceWidth) {
        LayoutInflater inflater = (LayoutInflater) mAnchorView.getContext().getSystemService(
                Context.LAYOUT_INFLATER_SERVICE);
        View popupView = inflater.inflate(R.layout.rewards_on_boarding, null);

        setStartUsingButtonClick(popupView, deviceWidth);
        setHowDoseItWorkButtonClick(popupView);

        mPopupWindow.setWidth(deviceWidth);
        mPopupWindow.setContentView(popupView);
    }

    public void showLikePopDownMenu() {
        mPopupWindow.setTouchable(true);
        mPopupWindow.setFocusable(true);
        mPopupWindow.setOutsideTouchable(true);

        mPopupWindow.showAsDropDown(mAnchorView, 0, 0);
    }

    private void setStartUsingButtonClick(View view, int deviceWidth) {
        View startUsingButton = view.findViewById(R.id.start_using_rewards_button);
        startUsingButton.setOnClickListener(v -> {
            mPopupWindow.dismiss();
            RewardsOnBoardingLocationChoose panel =
                    new RewardsOnBoardingLocationChoose(mAnchorView, deviceWidth);
            panel.showLikePopDownMenu();
        });
    }

    private void setHowDoseItWorkButtonClick(View view) {
        View howDoseItWorkButton = view.findViewById(R.id.how_does_it_work);
        howDoseItWorkButton.setOnClickListener(v -> {
            mPopupWindow.dismiss();
            TabUtils.openUrlInNewTab(false, BraveRewardsPanel.REWARDS_TOUR_URL);
        });
    }
}
