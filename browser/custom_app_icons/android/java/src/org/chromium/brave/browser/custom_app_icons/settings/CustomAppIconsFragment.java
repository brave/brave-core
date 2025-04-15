/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
package org.chromium.brave.browser.custom_app_icons.settings;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.brave.browser.custom_app_icons.CustomAppIcons;
import org.chromium.brave.browser.custom_app_icons.CustomAppIcons.AppIconType;
import org.chromium.brave.browser.custom_app_icons.CustomAppIconsManager;
import org.chromium.brave.browser.custom_app_icons.R;
import org.chromium.brave.browser.custom_app_icons.confirm_dialog.BraveConfirmationDialog;
import org.chromium.brave.browser.custom_app_icons.confirm_dialog.OnConfirmationDialogListener;

public class CustomAppIconsFragment extends Fragment implements CustomAppIconsListener {
    private static final String TAG = "CustomAppIconsFragment";
    private RecyclerView mRecyclerView;

    @Override
    public View onCreateView(
            @NonNull LayoutInflater inflater,
            @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        return setupView(inflater.inflate(R.layout.fragment_custom_app_icons, container, false));
    }

    private View setupView(View view) {
        mRecyclerView = view.findViewById(R.id.custom_app_icons_recycler_view);
        mRecyclerView.setLayoutManager(new LinearLayoutManager(getContext()));
        mRecyclerView.setAdapter(createAdapter());
        return view;
    }

    private CustomAppIconsAdapter createAdapter() {
        return new CustomAppIconsAdapter(CustomAppIcons.getAllIcons(), this);
    }

    @Override
    public void onCustomAppIconSelected(@AppIconType int appIconType) {
        showConfirmationDialog(appIconType);
    }

    private void showConfirmationDialog(@AppIconType int appIconType) {
        OnConfirmationDialogListener listener = createDialogListener(appIconType);
        new BraveConfirmationDialog()
                .showConfirmDialog(
                        getContext(),
                        getString(R.string.change_app_icon),
                        getString(R.string.custom_app_icons_switch_icon_message),
                        getString(R.string.custom_app_icons_switch_icon_positive_button_text),
                        getString(R.string.custom_app_icons_switch_icon_negative_button_text),
                        listener);
    }

    private OnConfirmationDialogListener createDialogListener(@AppIconType int appIconType) {
        return new OnConfirmationDialogListener() {
            @Override
            public void onPositiveButtonClicked() {
                handleIconSwitch(appIconType);
            }

            @Override
            public void onNegativeButtonClicked() {
                // No action needed
            }
        };
    }

    private void handleIconSwitch(@AppIconType int appIconType) {
        CustomAppIconsManager.getInstance().switchIcon(appIconType);
        requireActivity().onBackPressed();
    }
}
