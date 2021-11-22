/**
 * Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser;

import android.content.DialogInterface;
import android.content.res.Configuration;
import android.os.Bundle;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.style.ForegroundColorSpan;
import android.util.DisplayMetrics;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.appcompat.widget.AppCompatImageView;
import androidx.fragment.app.DialogFragment;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.notifications.retention.RetentionNotificationUtil;
import org.chromium.chrome.browser.util.ConfigurationUtils;
import org.chromium.ui.base.DeviceFormFactor;

import java.util.Arrays;
import java.util.List;

public class DormantUsersEngagementDialogFragment extends DialogFragment {
    private static final List<String> mTexts =
            Arrays.asList(ContextUtils.getApplicationContext().getResources().getString(
                                  R.string.dormant_users_engagement_text_1),
                    ContextUtils.getApplicationContext().getResources().getString(
                            R.string.dormant_users_engagement_text_2),
                    ContextUtils.getApplicationContext().getResources().getString(
                            R.string.dormant_users_engagement_text_3));
    private static final List<Integer> mImages = Arrays.asList(
            R.drawable.ic_rocket, R.drawable.ic_brave_battery, R.drawable.ic_brave_mobiledata);
    private String notificationType;

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        setDialogParams();
    }

    @Override
    public void onResume() {
        super.onResume();
        getDialog().setOnKeyListener(new DialogInterface.OnKeyListener() {
            @Override
            public boolean onKey(android.content.DialogInterface dialog, int keyCode,
                    android.view.KeyEvent event) {
                if ((keyCode == android.view.KeyEvent.KEYCODE_BACK)) {
                    dismiss();
                    return true;
                } else
                    return false;
            }
        });
        setDialogParams();
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(
                R.layout.fragment_dormant_users_engagement_dialog, container, false);
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        TextView engagementText = view.findViewById(R.id.dormant_users_engagement_text);
        ImageView imageView = view.findViewById(R.id.image_view);

        if (notificationType.equals(RetentionNotificationUtil.DORMANT_USERS_DAY_14)) {
            engagementText.setText(mTexts.get(0));
            imageView.setImageResource(mImages.get(0));
        } else if (notificationType.equals(RetentionNotificationUtil.DORMANT_USERS_DAY_25)) {
            engagementText.setText(mTexts.get(1));
            imageView.setImageResource(mImages.get(1));
        } else if (notificationType.equals(RetentionNotificationUtil.DORMANT_USERS_DAY_40)) {
            engagementText.setText(mTexts.get(2));
            imageView.setImageResource(mImages.get(2));
        }

        Button doneButton = view.findViewById(R.id.btn_done);
        doneButton.setOnClickListener((new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (BraveActivity.getBraveActivity() != null) {
                    BraveActivity.getBraveActivity().handleBraveSetDefaultBrowserDialog();
                }
                dismiss();
            }
        }));

        Button notNowButton = view.findViewById(R.id.btn_not_now);
        notNowButton.setOnClickListener((new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                dismiss();
            }
        }));
    }

    public void setNotificationType(String notificationType) {
        this.notificationType = notificationType;
    }

    private void setDialogParams() {
        DisplayMetrics displayMetrics = new DisplayMetrics();
        getActivity().getWindowManager().getDefaultDisplay().getMetrics(displayMetrics);
        int mDeviceHeight = displayMetrics.heightPixels;
        int mDeviceWidth = displayMetrics.widthPixels;

        WindowManager.LayoutParams params = getDialog().getWindow().getAttributes();
        boolean isTablet = DeviceFormFactor.isNonMultiDisplayContextOnTablet(getActivity());
        boolean isLandscape = ConfigurationUtils.isLandscape(getActivity());
        if (isTablet) {
            params.width = (int) (0.5 * mDeviceWidth);
        } else {
            if (isLandscape) {
                params.width = (int) (0.5 * mDeviceWidth);
            } else {
                params.width = (int) (0.9 * mDeviceWidth);
            }
        }
        params.height = LinearLayout.LayoutParams.WRAP_CONTENT;
        getDialog().getWindow().setAttributes(params);
    }
}
