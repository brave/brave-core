/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments.onboarding_fragments;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.util.DataFilesComponentInstaller;

import java.util.Locale;

/**
 * A fragment to display progress of BraveWallet component download.
 * Used on all Brave Wallet Onboarding framgents.
 */
public class DownloadComponentProgressFragment extends Fragment {
    private static final String TAG = "DWCPF";

    private DataFilesComponentInstaller mDataFilesComponentInstaller;
    private TextView mComponentDownloadProgress;

    public DownloadComponentProgressFragment() {}

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_download_component_progress, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        mDataFilesComponentInstaller = new DataFilesComponentInstaller();
        mComponentDownloadProgress =
                view.findViewById(R.id.onboarding_component_download_progress_f);
        // mComponentDownloadProgress.setText("OMG!!!");

        setupDownloadProgress();
    }

    private void setupDownloadProgress() {
        mDataFilesComponentInstaller.setInfoCallback(
                new DataFilesComponentInstaller.InfoCallback() {
                    @Override
                    public void onInfo(String info) {
                        // TODO(AlexeyBarabash): this seems to be unused
                    }

                    @Override
                    public void onProgress(long downloadedBytes, long totalBytes) {
                        assert totalBytes > 0;
                        assert totalBytes < (long) Integer.MAX_VALUE;
                        assert downloadedBytes < (long) Integer.MAX_VALUE;

                        if (totalBytes > 0 && downloadedBytes == totalBytes) {
                            mComponentDownloadProgress.setVisibility(View.GONE);
                            return;
                        }
                        mComponentDownloadProgress.setVisibility(View.VISIBLE);
                        String progressText = String.format(Locale.ENGLISH,
                                "Downloading wallet data file \u2022 %d%%",
                                (int) (100 * downloadedBytes / totalBytes));
                        mComponentDownloadProgress.setText(progressText);
                    }
                    @Override
                    public void onDownloadUpdateComplete() {
                        mComponentDownloadProgress.setVisibility(View.GONE);
                    }
                });

        mDataFilesComponentInstaller.registerListener();
    }
}
