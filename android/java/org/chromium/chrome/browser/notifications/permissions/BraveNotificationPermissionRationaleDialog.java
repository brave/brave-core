/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.notifications.permissions;

import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.Button;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveDialogFragment;
import org.chromium.chrome.browser.notifications.BravePermissionUtils;

public class BraveNotificationPermissionRationaleDialog extends BraveDialogFragment {
    public static BraveNotificationPermissionRationaleDialog newInstance() {
        BraveNotificationPermissionRationaleDialog fragment =
                new BraveNotificationPermissionRationaleDialog();
        return fragment;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View view = inflater.inflate(
                R.layout.brave_notification_permission_rationale_dialog, container, false);
        if (getDialog() != null && getDialog().getWindow() != null) {
            getDialog().getWindow().setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));
            getDialog().getWindow().requestFeature(Window.FEATURE_NO_TITLE);
        }
        return view;
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        clickOnContinueButton(view);
        clickOnNotNow(view);
    }

    private void clickOnContinueButton(View view) {
        Button primaryButton = view.findViewById(R.id.notification_continue_button);
        primaryButton.setOnClickListener(v -> {
            dismiss();
            BravePermissionUtils.requestPermission(getActivity());
        });
    }

    private void clickOnNotNow(View view) {
        Button notNowButton = view.findViewById(R.id.notification_not_now_button);
        notNowButton.setOnClickListener(v -> { dismiss(); });
    }
}
