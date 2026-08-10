/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.model;

import android.util.Pair;
import android.util.SparseArray;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.lifecycle.ViewModel;

import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.chrome.browser.crypto_wallet.fragments.onboarding.OnboardingVerifyRecoveryPhraseFragment.VerificationStep;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Random;
import java.util.Set;

public class OnboardingViewModel extends ViewModel {
    boolean mLegacyRestoreEnabled;

    @Nullable private String mPassword;
    @Nullable private String mRecoveryPhrase;

    @NonNull final Set<NetworkInfo> mSelectedNetworks = new HashSet<>();
    @NonNull final Set<NetworkInfo> mAvailableNetworks = new HashSet<>();
    @NonNull final SparseArray<String> mVerificationWords = new SparseArray<>(3);

    // The unlock password, retained across configuration changes so the fragment (recreated fresh
    // after a rotation) can restore it.
    @Nullable private String mUnlockPassword;
    // Whether the user dismissed the biometric unlock prompt on the current unlock screen. Retained
    // across configuration changes so a rotation does not pop the prompt up again, but only for as
    // long as the hosting activity lives, so a freshly shown unlock screen enables it once more.
    private boolean mBiometricPromptDismissed;

    // Recovery phrase entry state for the restore wallet screen, retained across configuration
    // changes so the fragment, recreated fresh, can repopulate it. A word
    // count of zero means nothing has been captured yet.
    @NonNull private final List<String> mRestoreWalletWords = new ArrayList<>();
    private int mRestoreWalletWordCount;
    private int mRestoreWalletFocusedWordIndex = -1;
    private boolean mRestoreWalletLegacyEnabled;

    /** Stores the unlock password text so it survives a configuration change such as a rotation. */
    public void setUnlockPassword(@Nullable final String unlockPassword) {
        mUnlockPassword = unlockPassword;
    }

    /** Returns the unlock password to restore, or {@code null} if none is stored. */
    @Nullable
    public String getUnlockPassword() {
        return mUnlockPassword;
    }

    /** Records whether the user dismissed the biometric unlock prompt for the unlock screen. */
    public void setBiometricPromptDismissed(final boolean biometricPromptDismissed) {
        mBiometricPromptDismissed = biometricPromptDismissed;
    }

    /** Returns whether the user dismissed the biometric unlock prompt for the unlock screen. */
    public boolean isBiometricPromptDismissed() {
        return mBiometricPromptDismissed;
    }

    /**
     * Drops the captured unlock screen state so it is not re-applied the next time the unlock
     * screen is shown (for example after the wallet is unlocked and later re-locked). Called once
     * the wallet has been unlocked.
     */
    public void clearUnlockState() {
        mUnlockPassword = null;
        mBiometricPromptDismissed = false;
    }

    /** Stores the restore wallet screen state so it survives a configuration change. */
    public void saveRestoreWalletState(
            @NonNull final List<String> words,
            final int wordCount,
            final int focusedWordIndex,
            final boolean legacyEnabled) {
        mRestoreWalletWords.clear();
        mRestoreWalletWords.addAll(words);
        mRestoreWalletWordCount = wordCount;
        mRestoreWalletFocusedWordIndex = focusedWordIndex;
        mRestoreWalletLegacyEnabled = legacyEnabled;
    }

    /**
     * Drops any captured restore wallet state so it is not re-applied the next time the screen is
     * shown. Called once the recovery phrase has been submitted.
     */
    public void clearRestoreWalletState() {
        mRestoreWalletWords.clear();
        mRestoreWalletWordCount = 0;
        mRestoreWalletFocusedWordIndex = -1;
        mRestoreWalletLegacyEnabled = false;
    }

    /** Returns the recovery phrase words to restore, in visible order. */
    @NonNull
    public List<String> getRestoreWalletWords() {
        return mRestoreWalletWords;
    }

    /** Returns the restored recovery phrase word count, or {@code 0} if nothing was captured. */
    public int getRestoreWalletWordCount() {
        return mRestoreWalletWordCount;
    }

    /** Returns the index of the focused recovery phrase word to restore, or {@code -1} if none. */
    public int getRestoreWalletFocusedWordIndex() {
        return mRestoreWalletFocusedWordIndex;
    }

    /** Returns whether the legacy wallet import option was enabled on the restore screen. */
    public boolean isRestoreWalletLegacyEnabled() {
        return mRestoreWalletLegacyEnabled;
    }

    public void setLegacyRestoreEnabled(final boolean legacyRestoreEnabled) {
        mLegacyRestoreEnabled = legacyRestoreEnabled;
    }

    public boolean isLegacyRestoreEnabled() {
        return mLegacyRestoreEnabled;
    }

    public void setRecoveryPhrase(@NonNull final String recoveryPhrase) {
        mRecoveryPhrase = recoveryPhrase;
    }

    @Nullable
    public String getRecoveryPhrase() {
        return mRecoveryPhrase;
    }

    @NonNull
    public String requireRecoveryPhrase() {
        assert mRecoveryPhrase != null : "Wallet recovery phrase must not be null.";
        return mRecoveryPhrase;
    }

    public void setPassword(@NonNull final String password) {
        mPassword = password;
    }

    @NonNull
    public String getPassword() {
        assert mPassword != null : "Wallet password must not be null.";
        return mPassword;
    }

    @Nullable
    public Pair<Integer, String> getVerificationStep(@NonNull final VerificationStep step) {
        assert mVerificationWords.size() != 0 : "Verification word list must not be empty.";
        if (mVerificationWords.size() <= step.getValue()) {
            return null;
        }
        return extractPositionAndWordAtIndex(step.getValue());
    }

    @NonNull
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
