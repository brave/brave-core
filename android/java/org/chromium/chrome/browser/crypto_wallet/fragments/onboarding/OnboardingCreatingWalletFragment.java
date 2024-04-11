/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments.onboarding;

import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import org.chromium.brave_wallet.mojom.BraveWalletP3a;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.brave_wallet.mojom.OnboardingAction;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.domain.KeyringModel;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;

import java.util.Set;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

/** Onboarding fragment for Brave Wallet which shows the spinner while wallet is created/restored */
public class OnboardingCreatingWalletFragment extends BaseOnboardingWalletFragment {

    private static final int NEXT_PAGE_DELAY_MS = 700;

    private volatile boolean mAddTransitionDelay = true;

    @Override
    public View onCreateView(
            @NonNull LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_creating_wallet, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        setAnimatedBackground(view.findViewById(R.id.creating_wallet_root));
    }

    @Override
    public void onResume() {
        super.onResume();

        final ScheduledExecutorService scheduler = Executors.newScheduledThreadPool(1);

        scheduler.schedule(
                () -> {
                    mAddTransitionDelay = false;
                },
                NEXT_PAGE_DELAY_MS,
                TimeUnit.MILLISECONDS);

        KeyringModel keyringModel = getKeyringModel();
        if (keyringModel != null) {
            // Check if a wallet is already present and skip if that's the case.
            keyringModel.isWalletCreated(
                    isCreated -> {
                        if (isCreated) {
                            requireActivity().finish();
                            return;
                        }
                        createWallet(keyringModel);
                    });
        }
    }

    private void createWallet(@NonNull final KeyringModel keyringModel) {
        BraveWalletP3a braveWalletP3A = getBraveWalletP3A();
        JsonRpcService jsonRpcService = getJsonRpcService();

        if (jsonRpcService != null) {
            Set<NetworkInfo> availableNetworks = mOnboardingViewModel.getAvailableNetworks();
            Set<NetworkInfo> selectedNetworks = mOnboardingViewModel.getSelectedNetworks();
            keyringModel.createWallet(
                    mOnboardingViewModel.getPassword(),
                    availableNetworks,
                    selectedNetworks,
                    jsonRpcService,
                    recoveryPhrases -> {
                        if (braveWalletP3A != null) {
                            braveWalletP3A.reportOnboardingAction(OnboardingAction.RECOVERY_SETUP);
                        }

                        Utils.setCryptoOnboarding(false);

                        // Go to the next page after wallet creation is done.
                        if (mOnNextPage != null) {
                            // Add small delay if the Wallet creation completes faster than {@code
                            // NEXT_PAGE_DELAY_MS}.
                            if (mAddTransitionDelay) {
                                Handler handler = new Handler(Looper.getMainLooper());
                                handler.postDelayed(
                                        () -> mOnNextPage.gotoNextPage(), NEXT_PAGE_DELAY_MS);
                            } else {
                                mOnNextPage.gotoNextPage();
                            }
                        }
                    });
        }
    }

    @Override
    protected boolean canBeClosed() {
        return false;
    }

    @Override
    protected boolean canNavigateBack() {
        return false;
    }
}
