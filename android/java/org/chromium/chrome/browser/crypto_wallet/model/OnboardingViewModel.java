/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.model;

import org.chromium.chrome.browser.crypto_wallet.fragments.onboarding.OnboardingVerifyRecoveryPhraseFragment.VerificationStep;

import android.util.Pair;
import android.util.SparseArray;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.lifecycle.ViewModel;

import org.chromium.brave_wallet.mojom.NetworkInfo;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Random;
import java.util.Set;

public class OnboardingViewModel extends ViewModel {
    @Nullable private String mPassword;

    @NonNull final Set<NetworkInfo> mSelectedNetworks = new HashSet<>();
    @NonNull final Set<NetworkInfo> mAvailableNetworks = new HashSet<>();
    @NonNull final SparseArray<String> mVerificationWords = new SparseArray<>(3);

    public void setPassword(@NonNull final String password) {
        mPassword = password;
    }

    @NonNull
    public String getPassword() {
        if (mPassword == null) {
            throw new IllegalStateException("Wallet password must not be null.");
        }
        return mPassword;
    }

    @NonNull
    public Pair<Integer, String> getVerificationStep(@NonNull final VerificationStep step) {
        if (mVerificationWords.size() == 0) {
            throw new IllegalStateException("Verification word list must not be null.");
        }

        return extractPositionAndWordAtIndex(step.getValue());
    }

    private Pair<Integer, String> extractPositionAndWordAtIndex(final int index) {
        final int key = mVerificationWords.keyAt(index);
        return new Pair<>(key, mVerificationWords.get(key));
    }

    public void setSelectedNetworks(
            @NonNull final Set<NetworkInfo> selectedNetworks,
            @NonNull final Set<NetworkInfo> availableNetworks) {
        mSelectedNetworks.clear();
        mSelectedNetworks.addAll(selectedNetworks);

        mAvailableNetworks.clear();
        mAvailableNetworks.addAll(availableNetworks);
    }

    @NonNull
    public Set<NetworkInfo> getSelectedNetworks() {
        return mSelectedNetworks;
    }

    @NonNull
    public Set<NetworkInfo> getAvailableNetworks() {
        return mAvailableNetworks;
    }

    public void generateVerificationWords(@NonNull final List<String> recoveryPhrases) {
        final int wordsToChoose = 3;
        final int size = recoveryPhrases.size();
        assert size >= wordsToChoose : "Recovery phrase is less than three words.";

        mVerificationWords.clear();
        Random random = new Random();
        // Ensure we only add unique random indexes.
        List<Integer> chosenIndexes = new ArrayList<>();

        while (chosenIndexes.size() < wordsToChoose) {
            int randomIndex = random.nextInt(size);
            if (!chosenIndexes.contains(randomIndex)) {
                chosenIndexes.add(randomIndex);
                mVerificationWords.put(randomIndex, recoveryPhrases.get(randomIndex));
            }
        }
    }
}
