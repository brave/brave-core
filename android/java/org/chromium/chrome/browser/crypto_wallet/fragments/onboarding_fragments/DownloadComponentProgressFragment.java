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

import org.chromium.base.TimeUtils.ElapsedRealtimeMillisTimer;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.component_updater.BraveComponentUpdater;
import org.chromium.chrome.browser.crypto_wallet.util.WalletDataFilesInstaller;

/**
 * A fragment to display progress of Brave Wallet data files component
 * download. Used on all Brave Wallet Onboarding fragments.
 */
public class DownloadComponentProgressFragment extends Fragment {
    private static final String WALLET_COMPONENT_ID =
            WalletDataFilesInstaller.getWalletDataFilesComponentId();
    private TextView mComponentDownloadProgress;
    private BraveComponentUpdater.ComponentUpdaterListener mComponentUpdaterListener;
    private ElapsedRealtimeMillisTimer mGracePeriodNoDisplayTimer;
    private static final long GRACE_NO_DISPLAY_MSEC = 1000;

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_download_component_progress, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        mComponentDownloadProgress = view.findViewById(R.id.download_progress_text_view);
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

        mComponentUpdaterListener = (event, id) -> {
            if (!WALLET_COMPONENT_ID.equals(id)) {
                return;
            }
            BraveComponentUpdater.CrxUpdateItem crxUpdateItem =
                    BraveComponentUpdater.get().getUpdateState(
                            WalletDataFilesInstaller.getWalletDataFilesComponentId());
            showHideProgressByItem(crxUpdateItem);
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

        // On fast internet connection download goes quickly, and the text view can be
        // shown for only ~0.3 sec, that doesn't have much sense and may look strange.
        // We will use the timer and don't display text view for 1 sec so if it
        // complete in 1 sec - we will not show it at all.
        if (mGracePeriodNoDisplayTimer == null) {
            mGracePeriodNoDisplayTimer = new ElapsedRealtimeMillisTimer();
        }

        if (mGracePeriodNoDisplayTimer.getElapsedMillis() < GRACE_NO_DISPLAY_MSEC) {
            return;
        }

        updateItem.normalizeZeroProgress();

        mComponentDownloadProgress.setVisibility(View.VISIBLE);

        assert updateItem.mTotalBytes != 0;
        String progressText = String.format(getString(R.string.download_wallet_data_files_progress),
                (int) (100 * updateItem.mDownloadedBytes / updateItem.mTotalBytes));

        mComponentDownloadProgress.setText(progressText);
    }
}
