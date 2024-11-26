/**
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.rewards.tipping;

import android.content.res.Configuration;
import android.content.res.Resources;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.os.Bundle;
import android.text.SpannableString;
import android.text.method.LinkMovementMethod;
import android.util.DisplayMetrics;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.fragment.app.DialogFragment;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.util.ConfigurationUtils;

public class PopupWindowTippingTabletUI extends DialogFragment {
    private static final String TAG = "TippingBanner";
    private static final String ANCHOR_Y_POSITION = "y_position";

    private int mCurrentTabId = -1;

    public static PopupWindowTippingTabletUI newInstance(View anchorView, int currentTabId) {
        int[] location = new int[2];
        anchorView.getLocationOnScreen(location);
        int popup_y_position = location[1] + anchorView.getHeight() - 40;
        PopupWindowTippingTabletUI fragment = new PopupWindowTippingTabletUI();
        Bundle args = new Bundle();
        args.putInt(RewardsTippingBannerActivity.TAB_ID_EXTRA, currentTabId);
        args.putInt(ANCHOR_Y_POSITION, popup_y_position);
        fragment.setArguments(args);
        return fragment;
    }

    @Override
    public void onResume() {
        super.onResume();
        setDialogParams();
    }

    private void setDialogParams() {
        DisplayMetrics displayMetrics = new DisplayMetrics();
        getActivity().getWindowManager().getDefaultDisplay().getMetrics(displayMetrics);
        int mDeviceWidth = displayMetrics.widthPixels;
        Window window = getDialog().getWindow();

        WindowManager.LayoutParams params = window.getAttributes();
        boolean isLandscape = ConfigurationUtils.isLandscape(getActivity());
        double widthRatio = isLandscape ? 0.8 : 0.96;
        params.width = (int) (widthRatio * mDeviceWidth);
        params.height = LinearLayout.LayoutParams.WRAP_CONTENT;
        window.setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));
        if (getArguments() != null) {
            params.y = getArguments().getInt(ANCHOR_Y_POSITION);
        }

        params.gravity = Gravity.TOP;
        params.flags &= ~WindowManager.LayoutParams.FLAG_DIM_BEHIND;
        window.setAttributes(params);
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.activity_tipping_banner_tablet, container, false);

        if (getArguments() != null) {
            mCurrentTabId = getArguments().getInt(RewardsTippingBannerActivity.TAB_ID_EXTRA);
        }
        replace();
        updateTermsOfServicePlaceHolder(view);
        return view;
    }

    private void updateTermsOfServicePlaceHolder(View view) {
        Resources res = getResources();
        TextView proceedTextView = view.findViewById(R.id.proceed_terms_of_service);
        proceedTextView.setMovementMethod(LinkMovementMethod.getInstance());
        String termsOfServiceText = String.format(res.getString(R.string.brave_rewards_tos_text),
                res.getString(R.string.terms_of_service), res.getString(R.string.privacy_policy));

        SpannableString spannableString = BraveRewardsHelper.tosSpannableString(
                termsOfServiceText, R.color.terms_of_service_text_color);
        proceedTextView.setText(spannableString);
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        if (isRemoving() || isDetached()) {
            return;
        }
        setDialogParams();
    }

    public void replace() {
        RewardsTippingBannerFragment tippingBannerFragment =
                RewardsTippingBannerFragment.newInstance(mCurrentTabId);
        getChildFragmentManager()
                .beginTransaction()
                .replace(R.id.tippingBannerFragment, tippingBannerFragment)
                .commit();
        RewardsTippingPanelFragment tippingPanelFragment =
                RewardsTippingPanelFragment.newInstance(mCurrentTabId);
        getChildFragmentManager()
                .beginTransaction()
                .replace(R.id.tippingPanelFragment2, tippingPanelFragment)
                .commit();
    }
}
