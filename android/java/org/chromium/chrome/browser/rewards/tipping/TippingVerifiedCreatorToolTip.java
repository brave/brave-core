/**
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.rewards.tipping;

import static org.chromium.ui.base.ViewUtils.dpToPx;

import android.annotation.SuppressLint;
import android.content.Context;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.method.LinkMovementMethod;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.widget.PopupWindow;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.appcompat.content.res.AppCompatResources;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.customtabs.CustomTabActivity;
import org.chromium.ui.text.ChromeClickableSpan;

public class TippingVerifiedCreatorToolTip {
    private static final String NEW_SIGNUP_DISABLED_URL =
            "https://support.brave.com/hc/en-us/articles/9312922941069";

    private PopupWindow mPopupWindow;
    public TippingVerifiedCreatorToolTip(@NonNull Context context) {
        init(context);
    }

    @SuppressLint("ClickableViewAccessibility")
    private void init(Context context) {
        LayoutInflater inflater =
                (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        View contentView = inflater.inflate(R.layout.tipping_verified_creator_tooltip, null, false);
        setLearnMore(contentView);
        mPopupWindow = new PopupWindow(context);
        mPopupWindow.setContentView(contentView);
        mPopupWindow.setTouchable(true);
        mPopupWindow.setFocusable(true);
        mPopupWindow.setBackgroundDrawable(AppCompatResources.getDrawable(
                context, R.drawable.tipping_verified_creator_tooltip_background));
        mPopupWindow.setElevation(60);
        mPopupWindow.setWidth((int) dpToPx(context, 280.0f));
        mPopupWindow.setTouchInterceptor((v, event) -> {
            if (event.getAction() == MotionEvent.ACTION_OUTSIDE) {
                mPopupWindow.dismiss();
                return true;
            }
            return false;
        });
    }

    private void setLearnMore(View view) {
        TextView descriptionTextView = view.findViewById(R.id.verified_creator_description);
        String descriptionText =
                String.format(view.getResources().getString(R.string.tipping_tooltip_description),
                        view.getResources().getString(R.string.learn_more));
        SpannableString spannableLearnMore =
                learnMoreSpannableString(view.getContext(), descriptionText);
        descriptionTextView.setMovementMethod(LinkMovementMethod.getInstance());
        descriptionTextView.setText(spannableLearnMore);
    }

    private SpannableString learnMoreSpannableString(Context context, String text) {
        Spanned textToAgree = BraveRewardsHelper.spannedFromHtmlString(text);

        SpannableString ss = new SpannableString(textToAgree.toString());

        ChromeClickableSpan clickableSpan =
                new ChromeClickableSpan(
                        context,
                        R.color.brave_rewards_modal_theme_color,
                        (textView) -> {
                            CustomTabActivity.showInfoPage(context, NEW_SIGNUP_DISABLED_URL);
                        });

        int learnMoreIndex = text.indexOf(context.getResources().getString(R.string.learn_more));

        ss.setSpan(clickableSpan, learnMoreIndex,
                learnMoreIndex + context.getResources().getString(R.string.learn_more).length(),
                Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        return ss;
    }

    public void show(@NonNull View anchorView) {
        int[] location = new int[2];
        anchorView.getLocationInWindow(location);
        mPopupWindow.showAtLocation(
                anchorView, Gravity.CENTER_HORIZONTAL | Gravity.TOP, 0, location[1] + 80);
    }
}
