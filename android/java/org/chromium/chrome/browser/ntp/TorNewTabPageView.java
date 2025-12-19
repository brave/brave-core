/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp;

import android.content.Context;
import android.util.AttributeSet;
import android.view.View;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.TextView;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.tor.TorService;

/**
 * The New Tab Page for Private Windows with Tor.
 * Extends BraveNewTabPageLayout to fit into the existing NTP infrastructure,
 * but overrides behavior to show Tor-specific UI.
 */
public class TorNewTabPageView extends BraveNewTabPageLayout {
    private TextView mConnectionStatus;
    private Button mNewIdentityButton;
    private ProgressBar mProgressBar;

    public TorNewTabPageView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();

        mConnectionStatus = findViewById(R.id.tv_connection_status);
        mNewIdentityButton = findViewById(R.id.btn_new_identity);
        mProgressBar = findViewById(R.id.progress_connection);

        if (mNewIdentityButton != null) {
            mNewIdentityButton.setOnClickListener(v -> {
                TorService.getInstance().newIdentity();
                showConnecting();
            });
        }

        updateConnectionStatus();
    }

    private void showConnecting() {
        if (mConnectionStatus != null) {
            mConnectionStatus.setText(R.string.tor_status_switching);
        }
        if (mProgressBar != null) {
            mProgressBar.setVisibility(View.VISIBLE);
        }
    }

    public void updateConnectionStatus() {
        if (mConnectionStatus == null) return;

        boolean isConnected = TorService.getInstance().isConnected();
        if (isConnected) {
            mConnectionStatus.setText(R.string.tor_status_connected);
            if (mProgressBar != null) {
                mProgressBar.setVisibility(View.GONE);
            }
        } else {
            mConnectionStatus.setText(R.string.tor_status_connecting);
            if (mProgressBar != null) {
                mProgressBar.setVisibility(View.VISIBLE);
            }
        }
    }

    @Override
    protected void initializeSiteSectionView() {
        // Do nothing - Tor NTP doesn't need Top Sites
    }

    @Override
    public void checkForBraveStats() {
        // No stats on Tor page
    }
}
