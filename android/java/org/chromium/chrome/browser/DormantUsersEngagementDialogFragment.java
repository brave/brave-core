/**
 * Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.notifications.retention.RetentionNotificationUtil;
import org.chromium.chrome.browser.set_default_browser.BraveSetDefaultBrowserUtils;

import java.util.Arrays;
import java.util.List;

public class DormantUsersEngagementDialogFragment extends BraveDialogFragment {
    private static final String TAG = "DormantEngagement";

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
        doneButton.setOnClickListener(
                new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        try {
                            BraveSetDefaultBrowserUtils.setDefaultBrowser(
                                    BraveActivity.getBraveActivity());
                        } catch (BraveActivity.BraveActivityNotFoundException e) {
                            Log.e(TAG, "onViewCreated doneButton click " + e);
                        }
                        dismiss();
                    }
                });

        Button notNowButton = view.findViewById(R.id.btn_not_now);
        notNowButton.setOnClickListener(
                new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        dismiss();
                    }
                });
    }

    public void setNotificationType(String notificationType) {
        this.notificationType = notificationType;
    }
}
