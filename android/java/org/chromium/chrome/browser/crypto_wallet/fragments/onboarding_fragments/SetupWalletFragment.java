/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments.onboarding_fragments;

import android.content.Intent;
import android.os.Build;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.FragmentActivity;

import org.chromium.base.Log;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.helpers.Api33AndPlusBackPressHelper;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.crypto_wallet.util.WalletDataFilesInstaller;

/**
 * Fragment to setup Brave Wallet
 */
public class SetupWalletFragment extends CryptoOnboardingFragment {
    private static final String TAG = "SetupWalletFragment";

    private boolean mRestartSetupAction;
    private boolean mRestartRestoreAction;

    public SetupWalletFragment(boolean restartSetupAction, boolean restartRestoreAction) {
        mRestartSetupAction = restartSetupAction;
        mRestartRestoreAction = restartRestoreAction;
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            Api33AndPlusBackPressHelper.create(
                    this, (FragmentActivity) requireActivity(), () -> requireActivity().finish());
        }
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_setup_wallet, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        Button setupCryptoButton = view.findViewById(R.id.btn_setup_crypto);
        setupCryptoButton.setOnClickListener(v -> {
            checkOnBraveActivity(true, false);
            onNextPage.gotoOnboardingPage();
        });

        TextView restoreButton = view.findViewById(R.id.btn_restore);
        restoreButton.setOnClickListener(v -> {
            checkOnBraveActivity(false, true);
            onNextPage.gotoRestorePage(true);
        });
        PostTask.postTask(TaskTraits.UI_DEFAULT, () -> {
            if (mRestartSetupAction) {
                setupCryptoButton.performClick();
            } else if (mRestartRestoreAction) {
                restoreButton.performClick();
            }
            mRestartSetupAction = false;
            mRestartRestoreAction = false;
        });
        WalletDataFilesInstaller.registerWalletDataFilesComponentOnDemand();
    }

    // We need to remove that check and restart once
    // https://github.com/brave/brave-browser/issues/27887
    // is done.
    private void checkOnBraveActivity(boolean setupAction, boolean restoreAction) {
        try {
            BraveActivity.getBraveActivity();
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "checkOnBraveActivity " + e);
            Intent intent = new Intent(getActivity(), ChromeTabbedActivity.class);
            intent.putExtra(Utils.RESTART_WALLET_ACTIVITY, true);
            intent.putExtra(Utils.RESTART_WALLET_ACTIVITY_SETUP, setupAction);
            intent.putExtra(Utils.RESTART_WALLET_ACTIVITY_RESTORE, restoreAction);
            intent.addFlags(Intent.FLAG_ACTIVITY_REORDER_TO_FRONT);
            intent.setAction(Intent.ACTION_VIEW);
            startActivity(intent);
            getActivity().finish();
        }
    }
}
