/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.notifications;

import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.os.Bundle;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveDialogFragment;

public class RewardsYouAreNotEarningDialog extends BraveDialogFragment {
    public static final String RewardsYouAreNotEarningDialogTAG =
            "RewardsYouAreNotEarningDialogTag";

    public static RewardsYouAreNotEarningDialog newInstance() {
        RewardsYouAreNotEarningDialog fragment = new RewardsYouAreNotEarningDialog();
        Bundle args = new Bundle();
        fragment.setArguments(args);
        return fragment;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.rewards_you_are_not_earning_dialog, container, false);
        if (getDialog() != null && getDialog().getWindow() != null) {
            getDialog().getWindow().setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));
            getDialog().getWindow().requestFeature(Window.FEATURE_NO_TITLE);
            Window window = getDialog().getWindow();
            WindowManager.LayoutParams wlp = window.getAttributes();
            wlp.y = 50;
            wlp.gravity = Gravity.BOTTOM;
            wlp.flags &= ~WindowManager.LayoutParams.FLAG_DIM_BEHIND;
            window.setAttributes(wlp);
        }
        return view;
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        clickOnCloseButton(view);
        clickOnTurnOnButton(view);
    }

    private void clickOnCloseButton(View view) {
        ImageView closeButton = view.findViewById(R.id.notification_disabled_close_button);
        closeButton.setOnClickListener((v) -> { dismiss(); });
    }

    private void clickOnTurnOnButton(View view) {
        TextView turnOnButton = view.findViewById(R.id.turn_on_button);
        turnOnButton.setOnClickListener((v) -> {
            dismiss();
            BravePermissionUtils.notificationSettingPage(getContext());
        });
    }
}
