/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments.onboarding;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.lifecycle.ViewModelProvider;
import androidx.lifecycle.ViewModelStoreOwner;

import org.chromium.brave_wallet.mojom.BraveWalletP3a;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.brave_wallet.mojom.OnboardingAction;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.model.OnboardingViewModel;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;

/** Onboarding fragment for Brave Wallet which shows the spinner while wallet is created/restored */
public class OnboardingCreatingWalletFragment extends BaseOnboardingWalletFragment {
    private OnboardingViewModel mOnboardingViewModel;

    @Override
    public View onCreateView(
            @NonNull LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_creating_wallet, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        setAnimatedBackground(view.findViewById(R.id.creating_wallet_root));
        mOnboardingViewModel =
                new ViewModelProvider((ViewModelStoreOwner) requireActivity())
                        .get(OnboardingViewModel.class);

        KeyringService keyringService = getKeyringService();
        BraveWalletP3a braveWalletP3A = getBraveWalletP3A();
        if (keyringService != null) {
            mOnboardingViewModel
                    .getPassword()
                    .observe(
                            getViewLifecycleOwner(),
                            password ->
                                    keyringService.createWallet(
                                            password,
                                            recoveryPhrases -> {
                                                if (braveWalletP3A != null) {
                                                    braveWalletP3A.reportOnboardingAction(
                                                            OnboardingAction.RECOVERY_SETUP);
                                                }
                                                // Go to the next page after wallet creation is done
                                                Utils.setCryptoOnboarding(false);
                                                if (mOnNextPage != null) {
                                                    mOnNextPage.gotoNextPage();
                                                }
                                            }));
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
