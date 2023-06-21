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
import android.text.SpannableString;
import android.text.Spanned;
import android.text.method.LinkMovementMethod;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.PopupWindow;
import android.widget.TextView;

import androidx.core.content.res.ResourcesCompat;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.customtabs.CustomTabActivity;
import org.chromium.chrome.browser.rewards.BraveRewardsPanel;
import org.chromium.ui.text.NoUnderlineClickableSpan;

/**
 * This class is used for showing error while creating wallet
 * */
public class RewardsOnBoardingError {
    private final View mAnchorView;
    private final PopupWindow mPopupWindow;
    private String mErrorMessage;

    public RewardsOnBoardingError(View anchorView, int deviceWidth, String errorMessage) {
        mErrorMessage = errorMessage;
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
        View popupView = inflater.inflate(R.layout.rewards_on_boarding_error, null);

        setStartUsingButtonClick(popupView, deviceWidth);
        setCloseButtonClick(popupView);

        mPopupWindow.setWidth(deviceWidth);
        mPopupWindow.setContentView(popupView);
        updateDescription(popupView);
    }

    public void showLikePopDownMenu() {
        mPopupWindow.setTouchable(true);
        mPopupWindow.setFocusable(true);
        mPopupWindow.setOutsideTouchable(true);

        mPopupWindow.showAsDropDown(mAnchorView, 0, 0);
    }

    private void setStartUsingButtonClick(View view, int deviceWidth) {
        View startUsingButton = view.findViewById(R.id.response_action_btn);
        startUsingButton.setOnClickListener(v -> {
            mPopupWindow.dismiss();
            RewardsOnBoardingLocationChoose panel =
                    new RewardsOnBoardingLocationChoose(mAnchorView, deviceWidth);
            panel.showLikePopDownMenu();
        });
    }

    private void setCloseButtonClick(View view) {
        View startUsingButton = view.findViewById(R.id.response_modal_close);
        startUsingButton.setOnClickListener(v -> { mPopupWindow.dismiss(); });
    }

    private void updateDescription(View view) {
        TextView responseModalText = view.findViewById(R.id.rewards_onboarding_error_description);
        TextView responseRewardsBtn = view.findViewById(R.id.response_action_btn);
        TextView responseErrorText = view.findViewById(R.id.response_error_text);

        String actionText = view.getContext().getString(R.string.retry_text);
        if (mErrorMessage.equals(BraveRewardsPanel.WALLET_GENERATION_DISABLED_ERROR)) {
            String title =
                    view.getContext().getString(R.string.wallet_generation_disabled_error_title);
            String text = String.format(
                    view.getContext().getString(R.string.wallet_generation_disabled_error_text),
                    view.getContext().getResources().getString(R.string.learn_more));
            SpannableString spannableWithLearnMore =
                    learnMoreSpannableString(view.getContext(), text);
            responseModalText.setMovementMethod(LinkMovementMethod.getInstance());
            responseModalText.setText(spannableWithLearnMore);
            actionText = view.getContext().getString(R.string.close_text);
        }

        responseRewardsBtn.setText(actionText);
        responseErrorText.setText(mErrorMessage);
    }

    private SpannableString learnMoreSpannableString(Context context, String text) {
        Spanned textToAgree = BraveRewardsHelper.spannedFromHtmlString(text);

        SpannableString ss = new SpannableString(textToAgree.toString());

        NoUnderlineClickableSpan clickableSpan = new NoUnderlineClickableSpan(
                context, R.color.brave_rewards_modal_theme_color, (textView) -> {
                    CustomTabActivity.showInfoPage(
                            context, BraveRewardsPanel.NEW_SIGNUP_DISABLED_URL);
                });
        int learnMoreIndex = text.indexOf(context.getResources().getString(R.string.learn_more));

        ss.setSpan(clickableSpan, learnMoreIndex,
                learnMoreIndex + context.getResources().getString(R.string.learn_more).length(),
                Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        return ss;
    }
}
