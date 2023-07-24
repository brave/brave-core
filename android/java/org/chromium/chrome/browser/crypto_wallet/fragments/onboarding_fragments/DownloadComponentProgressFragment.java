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
import org.chromium.chrome.browser.component_updater.BraveComponentUpdater;
import org.chromium.chrome.browser.crypto_wallet.util.WalletDataFilesInstaller;

import java.util.Locale;

/**
 * A fragment to display progress of BraveWallet component download.
 * Used on all Brave Wallet Onboarding framgents.
 */
public class DownloadComponentProgressFragment extends Fragment {
    private static final String TAG = "DWCPF";
    private static final String WALLET_COMPONENT_ID =
            WalletDataFilesInstaller.getWalletDataFilesComponentId();
    private TextView mComponentDownloadProgress;
    BraveComponentUpdater.ComponentUpdaterListener mComponentUpdaterListener;

    public DownloadComponentProgressFragment() {}

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_download_component_progress, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        mComponentDownloadProgress =
                view.findViewById(R.id.download_progress_text_view);
        setupDownloadProgress();
    }

    @Override
    public void onDestroyView() {
        if (mComponentUpdaterListener != null) {
            BraveComponentUpdater.get().removeComponentUpdateEventListener(
                    mComponentUpdaterListener);
            mComponentUpdaterListener = null;
        }
        super.onDestroyView();
    }

    private void setupDownloadProgress() {
        BraveComponentUpdater.CrxUpdateItem updateItem = BraveComponentUpdater.get().getUpdateState(
                WalletDataFilesInstaller.getWalletDataFilesComponentId());
        showHideProgressByItem(updateItem);

        mComponentUpdaterListener = new BraveComponentUpdater.ComponentUpdaterListener() {
            @Override
            public void onComponentUpdateEvent(int event, String id) {
                if (!WALLET_COMPONENT_ID.equals(id)) {
                    return;
                }
                BraveComponentUpdater.CrxUpdateItem updateItem =
                        BraveComponentUpdater.get().getUpdateState(
                                WalletDataFilesInstaller.getWalletDataFilesComponentId());
                showHideProgressByItem(updateItem);
            }
        };

        BraveComponentUpdater.get().addComponentUpdateEventListener(mComponentUpdaterListener);
    }

    private void showHideProgressByItem(BraveComponentUpdater.CrxUpdateItem updateItem) {
        assert updateItem.mTotalBytes < (long) Integer.MAX_VALUE;
        assert updateItem.mDownloadedBytes < (long) Integer.MAX_VALUE;

        if (updateItem.mTotalBytes > 0 && updateItem.mDownloadedBytes == updateItem.mTotalBytes
                || !updateItem.isInProgress()) {
            mComponentDownloadProgress.setVisibility(View.GONE);
            return;
        }

        updateItem.normalizeZeroProgress();

        mComponentDownloadProgress.setVisibility(View.VISIBLE);
        String progressText =
                String.format(Locale.ENGLISH, "Downloading wallet data file \u2022 %d%%",
                        (int) (100 * updateItem.mDownloadedBytes / updateItem.mTotalBytes));
        mComponentDownloadProgress.setText(progressText);
    }
}
