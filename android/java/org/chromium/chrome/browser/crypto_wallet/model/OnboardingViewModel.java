/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.model;

import android.util.Pair;
import android.util.SparseArray;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.ViewModel;

import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.chrome.browser.app.domain.KeyringModel;
import org.chromium.chrome.browser.crypto_wallet.fragments.onboarding.OnboardingVerifyRecoveryPhraseFragment.VerificationStep;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;

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

    // Terms of use screen checkbox selections, retained across configuration changes so the
    // fragment (recreated fresh after a rotation) can restore them.
    private boolean mSelfCustodyChecked;
    private boolean mTermsOfUseChecked;

    // Secure password screen entries, retained across configuration changes so the fragment,
    // recreated fresh after a rotation can restore them.
    @Nullable private String mSecurePassword;
    @Nullable private String mSecureRetypePassword;

    // Word typed on the current verify recovery phrase step, retained across configuration changes
    // so the fragment recreated fresh after a rotation can restore it.
    @Nullable private String mVerificationTypedWord;
    private int mVerificationTypedStep = -1;

    // Network selection screen in-progress state, retained across configuration changes so the
    // fragment recreated fresh after a rotation can restore it. A null selection set means
    // nothing has been captured yet, so the adapter keeps its default selection.
    @Nullable private Set<NetworkInfo> mNetworkSelectionSelected;
    private boolean mNetworkSelectionShowTestnets;
    @Nullable private String mNetworkSelectionSearchQuery;

    // Wallet creation request, owned by the model so it survives configuration changes and keeps
    // running while the activity is in the background. Triggered only once.
    private boolean mWalletCreationRequested;

    @NonNull
    private final MutableLiveData<Boolean> mWalletCreationSucceeded = new MutableLiveData<>();

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

    /**
     * Stores the terms of use screen checkbox selections so they survive a configuration change.
     */
    public void setTermsOfUseSelections(
            final boolean selfCustodyChecked, final boolean termsOfUseChecked) {
        mSelfCustodyChecked = selfCustodyChecked;
        mTermsOfUseChecked = termsOfUseChecked;
    }

    /** Returns whether the self custody checkbox was checked on the terms of use screen. */
    public boolean isSelfCustodyChecked() {
        return mSelfCustodyChecked;
    }

    /** Returns whether the terms of use checkbox was checked on the terms of use screen. */
    public boolean isTermsOfUseChecked() {
        return mTermsOfUseChecked;
    }

    /**
     * Drops the captured terms of use selections so they are not re-applied the next time the
     * screen is shown. Called once the user leaves the screen (by continuing, going back, or
     * closing).
     */
    public void clearTermsOfUseSelections() {
        mSelfCustodyChecked = false;
        mTermsOfUseChecked = false;
    }

    /** Stores the secure password screen entry so it survives a configuration change. */
    public void setSecurePasswordEntry(
            @Nullable final String securePassword, @Nullable final String secureRetypePassword) {
        mSecurePassword = securePassword;
        mSecureRetypePassword = secureRetypePassword;
    }

    /** Returns the password typed on the secure password screen, or {@code null} if none. */
    @Nullable
    public String getSecurePassword() {
        return mSecurePassword;
    }

    /** Returns the confirmation password typed on the secure password screen, or {@code null}. */
    @Nullable
    public String getSecureRetypePassword() {
        return mSecureRetypePassword;
    }

    /**
     * Live outcome of the Wallet creation or restoration request: {@code true} on success, {@code
     * false} on failure. Observers are notified once the request completes, including observers
     * that subscribe after completion (for example a fragment recreated by a rotation).
     */
    @NonNull
    public LiveData<Boolean> getWalletCreationSucceeded() {
        return mWalletCreationSucceeded;
    }

    /**
     * Creates or restores the Wallet exactly once, based on the state captured during onboarding.
     * Repeat calls (for example from a fragment recreated by a rotation) are ignored while the
     * request is running or after it has completed. The request is owned by the model, so it is not
     * cancelled when the activity is recreated or sent to the background; its outcome is delivered
     * through {@link #getWalletCreationSucceeded()}.
     */
    public void createOrRestoreWallet(
            @NonNull final KeyringModel keyringModel,
            @NonNull final JsonRpcService jsonRpcService,
            @Nullable final KeyringService keyringService,
            final boolean overridePreviousWallet) {
        if (mWalletCreationRequested) {
            return;
        }
        mWalletCreationRequested = true;
        keyringModel.isWalletCreated(
                isCreated -> {
                    // Skip creation when a wallet already exists, unless restoring over it from the
                    // unlock screen button.
                    if (isCreated && !overridePreviousWallet) {
                        mWalletCreationSucceeded.setValue(true);
                        return;
                    }
                    if (mRecoveryPhrase == null) {
                        keyringModel.createWallet(
                                getPassword(),
                                mAvailableNetworks,
                                mSelectedNetworks,
                                jsonRpcService,
                                recoveryPhrases -> {
                                    Utils.setCryptoOnboarding(false);
                                    mWalletCreationSucceeded.setValue(true);
                                });
                    } else {
                        keyringModel.restoreWallet(
                                getPassword(),
                                requireRecoveryPhrase(),
                                mLegacyRestoreEnabled,
                                mAvailableNetworks,
                                mSelectedNetworks,
                                jsonRpcService,
                                result -> {
                                    if (result) {
                                        if (keyringService != null) {
                                            keyringService.notifyWalletBackupComplete();
                                        }
                                        Utils.setCryptoOnboarding(false);
                                    }
                                    mWalletCreationSucceeded.setValue(result);
                                });
                    }
                });
    }

    /** Clears every captured value so a new pass through onboarding starts from a clean state. */
    public void reset() {
        mLegacyRestoreEnabled = false;
        mPassword = null;
        mRecoveryPhrase = null;
        mSelectedNetworks.clear();
        mAvailableNetworks.clear();
        mVerificationWords.clear();
        clearUnlockState();
        clearRestoreWalletState();
        clearTermsOfUseSelections();
        mSecurePassword = null;
        mSecureRetypePassword = null;
        mVerificationTypedWord = null;
        mVerificationTypedStep = -1;
        mNetworkSelectionSelected = null;
        mNetworkSelectionShowTestnets = false;
        mNetworkSelectionSearchQuery = null;
        mWalletCreationRequested = false;
        mWalletCreationSucceeded.setValue(null);
    }

    public void setLegacyRestoreEnabled(final boolean legacyRestoreEnabled) {
        mLegacyRestoreEnabled = legacyRestoreEnabled;
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

    /** Stores the word typed on the given verify recovery phrase step so it survives a rotation. */
    public void setVerificationTypedWord(final int step, @Nullable final String typedWord) {
        mVerificationTypedStep = step;
        mVerificationTypedWord = typedWord;
    }

    /**
     * Returns the word typed on the given verify recovery phrase step, or {@code null} if the
     * stored word belongs to a different step or nothing is stored.
     */
    @Nullable
    public String getVerificationTypedWord(final int step) {
        return mVerificationTypedStep == step ? mVerificationTypedWord : null;
    }

    public void setSelectedNetworks(
            @NonNull final Set<NetworkInfo> selectedNetworks,
            @NonNull final Set<NetworkInfo> availableNetworks) {
        mSelectedNetworks.clear();
        mSelectedNetworks.addAll(selectedNetworks);

        mAvailableNetworks.clear();
        mAvailableNetworks.addAll(availableNetworks);
    }

    /** Stores the network selection screen state so it survives a rotation. */
    public void setNetworkSelectionState(
            @NonNull final Set<NetworkInfo> selectedNetworks,
            final boolean showTestnets,
            @Nullable final String searchQuery) {
        mNetworkSelectionSelected = new HashSet<>(selectedNetworks);
        mNetworkSelectionShowTestnets = showTestnets;
        mNetworkSelectionSearchQuery = searchQuery;
    }

    /**
     * Returns the selected networks captured on the network selection screen, or {@code null} if
     * nothing has been captured yet.
     */
    @Nullable
    public Set<NetworkInfo> getNetworkSelectionSelected() {
        return mNetworkSelectionSelected;
    }

    /** Returns whether the show testnets checkbox was checked on the network selection screen. */
    public boolean isNetworkSelectionShowTestnets() {
        return mNetworkSelectionShowTestnets;
    }

    /** Returns the search query typed on the network selection screen, or {@code null} if none. */
    @Nullable
    public String getNetworkSelectionSearchQuery() {
        return mNetworkSelectionSearchQuery;
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
