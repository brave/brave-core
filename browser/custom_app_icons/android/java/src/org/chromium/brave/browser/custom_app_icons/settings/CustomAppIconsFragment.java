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

import org.chromium.brave.browser.custom_app_icons.CustomAppIconsEnum;
import org.chromium.brave.browser.custom_app_icons.CustomAppIconsManager;
import org.chromium.brave.browser.custom_app_icons.R;

public class CustomAppIconsFragment extends Fragment implements CustomAppIconsListener {

    private RecyclerView mRecyclerView;

    @Override
    public View onCreateView(
            @NonNull LayoutInflater inflater,
            @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.fragment_custom_app_icons, container, false);

        mRecyclerView = view.findViewById(R.id.custom_app_icons_recycler_view);
        mRecyclerView.setLayoutManager(new LinearLayoutManager(getContext()));
        mRecyclerView.setAdapter(new CustomAppIconsAdapter(CustomAppIconsEnum.values(), this));

        return view;
    }

    @Override
    public void onCustomAppIconSelected(CustomAppIconsEnum icon) {
        CustomAppIconsManager.switchIcon(requireActivity(), icon);
        requireActivity().onBackPressed();
    }
}
