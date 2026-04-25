/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;
import org.chromium.chrome.browser.util.TabUtils;

public class OpenYtInBraveDialogFragment extends BraveDialogFragment
        implements View.OnClickListener {

    public static final String YOUTUBE_URL = "https://youtube.com";

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_open_yt_in_brave_dialog, container, false);
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        Button mDoneButton = view.findViewById(R.id.btn_done);
        mDoneButton.setOnClickListener(this);

        TextView mIgnoreButton = view.findViewById(R.id.btn_ignore);
        mIgnoreButton.setOnClickListener(this);
    }

    @Override
    public void onClick(View view) {
        if (view.getId() == R.id.btn_done) {
            BravePrefServiceBridge.getInstance().setPlayYTVideoInBrowserEnabled(true);
            TabUtils.openUrlInSameTab(YOUTUBE_URL);
        }
        dismiss();
    }
}
