/**
 * Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.notifications;

import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.app.ActivityCompat;

import org.chromium.base.BuildInfo;
import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveDialogFragment;
import org.chromium.chrome.browser.notifications.BravePermissionUtils;
import org.chromium.chrome.browser.onboarding.OnboardingPrefManager;
import org.chromium.ui.permissions.PermissionConstants;

/**
 * This dialog is used to show different messages when notification permission is off and
 * if rewards or privacy is on OR both on
 * */
public class BraveNotificationWarningDialog extends BraveDialogFragment {
    public static final String NOTIFICATION_WARNING_DIALOG_TAG = "NotificationWarningDialog";

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.brave_notification_warning_dialog, container, false);
        if (getDialog() != null && getDialog().getWindow() != null) {
            getDialog().getWindow().setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));
            getDialog().getWindow().requestFeature(Window.FEATURE_NO_TITLE);
        }
        return inflater.inflate(R.layout.brave_notification_warning_dialog, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        init(view);
    }

    private void init(View view) {
        updateTitleDescriptionText(view);
        clickOnPrimaryButton(view);
        clickOnCloseButton(view);
        clickOnNotNow(view);
    }

    public boolean isBraveRewardsEnabled() {
        return OnboardingPrefManager.getInstance().isBraveRewardsEnabled();
    }

    public boolean isPrivacyReportsEnabled() {
        return OnboardingPrefManager.getInstance().isBraveStatsEnabled();
    }

    private void updateTitleDescriptionText(View view) {
        TextView titleTextView = view.findViewById(R.id.notification_title_tv);
        TextView descriptionTextView = view.findViewById(R.id.notification_description_tv);

        if (isBraveRewardsEnabled() && isPrivacyReportsEnabled()) {
            titleTextView.setText(R.string.notification_os_dialog_header_both_rewards_privacy);
            descriptionTextView.setText(
                    R.string.notification_os_dialog_description_both_rewards_privacy);
        } else if (isBraveRewardsEnabled()) {
            titleTextView.setText(R.string.notification_os_dialog_header_only_rewards);
            descriptionTextView.setText(R.string.notification_os_dialog_description_only_rewards);
        } else if (isPrivacyReportsEnabled()) {
            titleTextView.setText(R.string.notification_os_dialog_header_only_privacy);
            descriptionTextView.setText(R.string.notification_os_dialog_description_only_privacy);
        }
    }

    private void clickOnPrimaryButton(View view) {
        Button primaryButton = view.findViewById(R.id.notification_warning_primary_button);
        primaryButton.setOnClickListener(v -> {
            dismiss();

            if (getActivity().shouldShowRequestPermissionRationale(
                        PermissionConstants.NOTIFICATION_PERMISSION)
                    || (!BuildInfo.isAtLeastT() || !BuildInfo.targetsAtLeastT())) {
                // other than android 13 redirect to
                // setting page and for android 13 Last time don't allow selected in permission
                // dialog, then enable through setting
                BravePermissionUtils.notificationSettingPage(getContext());
            } else {
                // 1st time request permission
                ActivityCompat.requestPermissions(getActivity(),
                        new String[] {PermissionConstants.NOTIFICATION_PERMISSION}, 1);
            }
        });
    }

    private void clickOnCloseButton(View view) {
        ImageView btnClose = view.findViewById(R.id.notification_dialog_close);
        btnClose.setOnClickListener(v -> { dismiss(); });
    }

    private void clickOnNotNow(View view) {
        Button notNowButton = view.findViewById(R.id.btn_not_now);
        notNowButton.setOnClickListener(v -> { dismiss(); });
    }
}
