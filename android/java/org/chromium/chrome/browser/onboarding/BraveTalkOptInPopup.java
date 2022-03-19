/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.onboarding;

import android.annotation.SuppressLint;
import android.content.Context;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.os.Build;
import android.os.Handler;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.TextPaint;
import android.text.method.LinkMovementMethod;
import android.text.style.ClickableSpan;
import android.text.style.ForegroundColorSpan;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.PopupWindow;
import android.widget.TextView;

import androidx.appcompat.widget.AppCompatImageView;

import org.chromium.base.SysUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.customtabs.CustomTabActivity;

public class BraveTalkOptInPopup {
    private static final long AUTO_DISMISS_CONFIRMATION_INTERVAL = 3000;

    private final View mAnchor;
    private final BraveTalkOptInPopupListener mListener;
    private final LayoutInflater mInflater;
    private final ChromeTabbedActivity mActivity;
    private final PopupWindow mWindow;
    private final View mView;
    private final AppCompatImageView mOptInImage;
    private final Button mOptInButton;
    private final TextView mOptInPopupTitle;
    private final TextView mOptInPopupDescription;
    private final TextView mOptInPopupTos;
    private final Handler mHandler = new Handler();

    public BraveTalkOptInPopup(View anchor, BraveTalkOptInPopupListener listener) {
        mAnchor = anchor;
        mListener = listener;
        mInflater = (LayoutInflater) mAnchor.getContext().getSystemService(
                Context.LAYOUT_INFLATER_SERVICE);
        mActivity = BraveRewardsHelper.getChromeTabbedActivity();
        mWindow = createPopupWindow();

        mView = mInflater.inflate(R.layout.brave_talk_opt_in_layout, null);
        mWindow.setContentView(mView);

        mOptInImage = mView.findViewById(R.id.brave_talk_opt_in_image);
        mOptInButton = mView.findViewById(R.id.brave_talk_opt_in_button);
        mOptInPopupTitle = mView.findViewById(R.id.brave_talk_opt_in_title);
        mOptInPopupDescription = mView.findViewById(R.id.brave_talk_opt_in_description);
        mOptInPopupTos = mView.findViewById(R.id.brave_talk_opt_in_tos);

        initPopupContent();
    }

    public void showLikePopDownMenu() {
        mWindow.setAnimationStyle(R.style.EndIconMenuAnim);

        if (SysUtils.isLowEndDevice()) {
            mWindow.setAnimationStyle(0);
        }

        mWindow.showAsDropDown(mAnchor, /* xOffset */ 0, /* yOffset */ 0);
    }

    public void dismissPopup() {
        mWindow.dismiss();
    }

    private PopupWindow createPopupWindow() {
        PopupWindow window = new PopupWindow(mAnchor.getContext());
        window.setWidth(ViewGroup.LayoutParams.WRAP_CONTENT);
        window.setHeight(ViewGroup.LayoutParams.WRAP_CONTENT);
        window.setBackgroundDrawable(new ColorDrawable(Color.WHITE));
        window.setTouchable(true);
        window.setFocusable(true);
        window.setOutsideTouchable(true);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            window.setElevation(20);
        }

        window.setTouchInterceptor(new View.OnTouchListener() {
            @SuppressLint("ClickableViewAccessibility")
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                if (event.getAction() == MotionEvent.ACTION_OUTSIDE) {
                    dismissPopup();
                    return true;
                }
                return false;
            }
        });
        window.setOnDismissListener(new PopupWindow.OnDismissListener() {
            @Override
            public void onDismiss() {
                mHandler.removeCallbacksAndMessages(null);

                BraveActivity braveActivity = BraveRewardsHelper.getBraveActivity();
                if (braveActivity != null) {
                    braveActivity.onBraveTalkOptInPopupDismiss();
                }

                mListener.notifyTalkOptInPopupClosed();
            }
        });

        return window;
    }

    private void initPopupContent() {
        mOptInButton.setOnClickListener((new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                onBraveTalkOptInButtonClicked();
            }
        }));

        if (!BraveRewardsHelper.shouldShowBraveRewardsOnboardingModal()) {
            mOptInButton.setText(mView.getResources().getString(
                    R.string.brave_talk_turn_on_private_ads_button_text));
            mOptInPopupTitle.setText(
                    mView.getResources().getString(R.string.brave_talk_turn_on_private_ads_text));
            mOptInPopupDescription.setText(mView.getResources().getString(
                    R.string.brave_talk_private_ads_description_text));
        }

        initTosTextView(R.string.brave_talk_rewards_tos_privacy_policy_text);
    }

    private void initTosTextView(int tos_privacy_policy_resource_id) {
        String termsOfServiceString = mView.getResources().getString(R.string.terms_of_service);
        String privacyPolicyString = mView.getResources().getString(R.string.privacy_policy);
        String tosText =
                String.format(mView.getResources().getString(tos_privacy_policy_resource_id),
                        termsOfServiceString, privacyPolicyString);
        Spanned tosTextSpanned = BraveRewardsHelper.spannedFromHtmlString(tosText);
        SpannableString tosTextSS = new SpannableString(tosTextSpanned.toString());

        ClickableSpan tosClickableSpan = new ClickableSpan() {
            @Override
            public void onClick(View textView) {
                CustomTabActivity.showInfoPage(mActivity, BraveActivity.BRAVE_TERMS_PAGE);
                dismissPopup();
            }
            @Override
            public void updateDrawState(TextPaint ds) {
                super.updateDrawState(ds);
                ds.setUnderlineText(true);
            }
        };

        int termsOfServiceIndex = tosText.indexOf(termsOfServiceString);
        tosTextSS.setSpan(tosClickableSpan, termsOfServiceIndex,
                termsOfServiceIndex + termsOfServiceString.length(),
                Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        tosTextSS.setSpan(new ForegroundColorSpan(mView.getResources().getColor(
                                  R.color.brave_rewards_modal_theme_color)),
                termsOfServiceIndex, termsOfServiceIndex + termsOfServiceString.length(),
                Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);

        ClickableSpan privacyProtectionClickableSpan = new ClickableSpan() {
            @Override
            public void onClick(View textView) {
                CustomTabActivity.showInfoPage(mActivity, BraveActivity.BRAVE_PRIVACY_POLICY);
                dismissPopup();
            }
            @Override
            public void updateDrawState(TextPaint ds) {
                super.updateDrawState(ds);
                ds.setUnderlineText(true);
            }
        };

        int privacyPolicyIndex = tosText.indexOf(privacyPolicyString);
        tosTextSS.setSpan(privacyProtectionClickableSpan, privacyPolicyIndex,
                privacyPolicyIndex + privacyPolicyString.length(),
                Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        tosTextSS.setSpan(new ForegroundColorSpan(mView.getResources().getColor(
                                  R.color.brave_rewards_modal_theme_color)),
                privacyPolicyIndex, privacyPolicyIndex + privacyPolicyString.length(),
                Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);

        mOptInPopupTos.setMovementMethod(LinkMovementMethod.getInstance());
        mOptInPopupTos.setText(tosTextSS);
    }

    private void onBraveTalkOptInButtonClicked() {
        mListener.notifyAdsEnableButtonPressed();
        if (BraveRewardsHelper.shouldShowBraveRewardsOnboardingModal()) {
            // No need to show brave rewards opt-in modal.
            BraveRewardsHelper.updateBraveRewardsAppOpenCount();
            BraveRewardsHelper.setShowBraveRewardsOnboardingModal(false);

            // No need to show brave rewards quick tour.
            BraveRewardsHelper.setShowBraveRewardsOnboardingOnce(false);

            // Update status of brave rewards icon.
            BraveActivity braveActivity = BraveActivity.getBraveActivity();
            if (braveActivity != null) {
                braveActivity.hideRewardsOnboardingIcon();
            }
            OnboardingPrefManager.getInstance().setOnboardingShown(true);
        }
        showStartFreeCallLayout();
    }

    private void showStartFreeCallLayout() {
        mOptInImage.setImageResource(R.drawable.ic_check_circle);
        LinearLayout.LayoutParams params =
                (LinearLayout.LayoutParams) mOptInImage.getLayoutParams();
        params.topMargin = 40;
        params.bottomMargin = 40;
        mOptInImage.setLayoutParams(params);
        mOptInButton.setVisibility(View.GONE);
        mOptInPopupTitle.setText(
                mView.getResources().getString(R.string.brave_talk_start_free_call_text));
        mOptInPopupDescription.setText(
                mView.getResources().getString(R.string.brave_talk_tap_anywhere_text));
        mOptInPopupTos.setVisibility(View.VISIBLE);
        mOptInPopupTos.setTextSize(14);
        showQuickTourTextLink();

        mHandler.postDelayed(new Runnable() {
            @Override
            public void run() {
                dismissPopup();
            }
        }, AUTO_DISMISS_CONFIRMATION_INTERVAL);
    }

    private void showQuickTourTextLink() {
        String quickTourString = mActivity.getResources().getString(R.string.take_quick_tour);
        String learnMoreString = String.format(mActivity.getResources().getString(
                R.string.brave_talk_want_learn_more_text, quickTourString));
        Spanned learnMoreSpanned = BraveRewardsHelper.spannedFromHtmlString(learnMoreString);
        SpannableString learnMoreSS = new SpannableString(learnMoreSpanned.toString());

        ClickableSpan learnMoreClickableSpan = new ClickableSpan() {
            @Override
            public void onClick(View textView) {
                onQuickTourClicked();
            }
            @Override
            public void updateDrawState(TextPaint ds) {
                super.updateDrawState(ds);
                ds.setUnderlineText(true);
            }
        };

        int quickTourIndex = learnMoreString.indexOf(quickTourString);
        learnMoreSS.setSpan(learnMoreClickableSpan, quickTourIndex,
                quickTourIndex + quickTourString.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        learnMoreSS.setSpan(new ForegroundColorSpan(mActivity.getResources().getColor(
                                    R.color.brave_rewards_modal_theme_color)),
                quickTourIndex, quickTourIndex + quickTourString.length(),
                Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        mOptInPopupTos.setMovementMethod(LinkMovementMethod.getInstance());
        mOptInPopupTos.setText(learnMoreSS);
    }

    private void onQuickTourClicked() {
        BraveActivity braveActivity = BraveActivity.getBraveActivity();
        if (braveActivity != null) {
            BraveRewardsHelper.setShowBraveRewardsOnboardingOnce(true);
            braveActivity.openRewardsPanel();
        }

        dismissPopup();
    }
}
