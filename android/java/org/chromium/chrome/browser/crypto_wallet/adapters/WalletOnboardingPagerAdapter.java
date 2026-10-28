/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.adapters;

import static org.chromium.chrome.browser.crypto_wallet.fragments.onboarding.OnboardingVerifyRecoveryPhraseFragment.VerificationStep.FIRST;
import static org.chromium.chrome.browser.crypto_wallet.fragments.onboarding.OnboardingVerifyRecoveryPhraseFragment.VerificationStep.SECOND;
import static org.chromium.chrome.browser.crypto_wallet.fragments.onboarding.OnboardingVerifyRecoveryPhraseFragment.VerificationStep.THIRD;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentActivity;
import androidx.viewpager2.adapter.FragmentStateAdapter;

import org.chromium.chrome.browser.crypto_wallet.fragments.UnlockWalletFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.onboarding.OnboardingBackupWalletFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.onboarding.OnboardingConfirmationFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.onboarding.OnboardingCreatingWalletFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.onboarding.OnboardingFingerprintUnlockFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.onboarding.OnboardingInitWalletFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.onboarding.OnboardingNetworkSelectionFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.onboarding.OnboardingRecoveryPhraseFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.onboarding.OnboardingRestoreWalletFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.onboarding.OnboardingSecurePasswordFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.onboarding.OnboardingTermsOfUseFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.onboarding.OnboardingVerifyRecoveryPhraseFragment;

public class WalletOnboardingPagerAdapter extends FragmentStateAdapter {
    /** Wallet action types used to determine onboarding navigation sequence. */
    public enum WalletAction {
        // Initial onboarding action triggered to create a new Wallet or restore an existing one.
        ONBOARDING,
        // Password creation action part of the onboarding flow, triggered when creating a new
        // Wallet.
        PASSWORD_CREATION,
        // Unlock action type triggered when accessing the locked Wallet.
        UNLOCK,
        // Restore action part of the onboarding flow, triggered when restoring a Wallet.
        ONBOARDING_RESTORE,
        // Restore action triggered outside onboarding flow on a pre-existing wallet.
        RESTORE,
        //
        BACKUP
    }

    private static final long TERMS_OF_USE_PASSWORD_CREATION_ID = 999;
    private static final long TERMS_OF_USE_RESTORE_ID = 998;
    private static final long UNLOCK_ID = 997;

    private final boolean mRestartSetupAction;
    private final boolean mRestartRestoreAction;

    @NonNull private WalletAction mWalletAction;

    // The most recently created terms of use fragment, cached so the host can reach it without
    // scanning the fragment manager. Points at the live instance while it exists.
    @Nullable private OnboardingTermsOfUseFragment mTermsOfUseFragment;

    public WalletOnboardingPagerAdapter(
            @NonNull final FragmentActivity fragmentActivity,
            final boolean restartSetupAction,
            final boolean restartRestoreAction) {
        super(fragmentActivity);
        mRestartSetupAction = restartSetupAction;
        mRestartRestoreAction = restartRestoreAction;
        mWalletAction = WalletAction.UNLOCK;
    }

    public void setWalletAction(@NonNull final WalletAction walletAction) {
        if (walletAction == mWalletAction) {
            return;
        }
        mWalletAction = walletAction;

        notifyItemRangeChanged(0, getItemCount());
    }

    /**
     * Returns the current wallet action, which determines the onboarding navigation sequence.
     *
     * @return the current {@link WalletAction}.
     */
    @NonNull
    public WalletAction getWalletAction() {
        return mWalletAction;
    }

    /** Returns whether the init Wallet fragment is shown at the given position. */
    public boolean isInitWalletFragmentAt(final int position) {
        return position == 0
                && (mWalletAction == WalletAction.ONBOARDING
                        || mWalletAction == WalletAction.PASSWORD_CREATION
                        || mWalletAction == WalletAction.ONBOARDING_RESTORE);
    }

    /** Returns the most recently created terms of use fragment, or {@code null} if none exists. */
    @Nullable
    public OnboardingTermsOfUseFragment getTermsOfUseFragment() {
        return mTermsOfUseFragment;
    }

    @Override
    public long getItemId(int position) {
        if (position == 0 && mWalletAction == WalletAction.UNLOCK) {
            return UNLOCK_ID;
        }
        // The terms of use fragment is used by two different wallet actions,
        // and it's important to differentiate the IDs for not sharing their state,
        // so we are manually passing constants for these two cases.
        if (position == 1) {
            if (mWalletAction == WalletAction.PASSWORD_CREATION) {
                return TERMS_OF_USE_PASSWORD_CREATION_ID;
            }
            if (mWalletAction == WalletAction.ONBOARDING_RESTORE) {
                return TERMS_OF_USE_RESTORE_ID;
            }
        }
        // For all the other cases, falling back to their position
        // is enough.
        return super.getItemId(position);
    }

    @NonNull
    @Override
    public Fragment createFragment(int position) {
        switch (mWalletAction) {
            case WalletAction.ONBOARDING -> {
                if (position == 0) {
                    return new OnboardingInitWalletFragment(
                            mRestartSetupAction, mRestartRestoreAction);
                } else {
                    throw new IllegalStateException(
                            String.format(
                                    "No fragment found for ONBOARDING Wallet action at position"
                                            + " %d.",
                                    position));
                }
            }
            case WalletAction.PASSWORD_CREATION -> {
                final boolean isOnboarding = true;
                final boolean overridePreviousWallet = false;

                switch (position) {
                    case 0:
                        return new OnboardingInitWalletFragment(
                                mRestartSetupAction, mRestartRestoreAction);
                    case 1:
                        mTermsOfUseFragment = OnboardingTermsOfUseFragment.newInstance();
                        return mTermsOfUseFragment;
                    case 2:
                        return OnboardingNetworkSelectionFragment.newInstance();
                    case 3:
                        return new OnboardingSecurePasswordFragment();
                    case 4:
                        return new OnboardingFingerprintUnlockFragment();
                    case 5:
                        return OnboardingCreatingWalletFragment.newInstance(overridePreviousWallet);
                    case 6:
                        return OnboardingRecoveryPhraseFragment.newInstance(isOnboarding);
                    case 7:
                        return OnboardingVerifyRecoveryPhraseFragment.newInstance(
                                isOnboarding, FIRST);
                    case 8:
                        return OnboardingVerifyRecoveryPhraseFragment.newInstance(
                                isOnboarding, SECOND);
                    case 9:
                        return OnboardingVerifyRecoveryPhraseFragment.newInstance(
                                isOnboarding, THIRD);
                    case 10:
                        return new OnboardingConfirmationFragment();
                    default:
                        throw new IllegalStateException(
                                String.format(
                                        "No fragment found for PASSWORD_CREATION Wallet action at"
                                                + " position %d.",
                                        position));
                }
            }
            case WalletAction.ONBOARDING_RESTORE -> {
                final boolean overridePreviousWallet = false;
                switch (position) {
                    case 0:
                        return new OnboardingInitWalletFragment(
                                mRestartSetupAction, mRestartRestoreAction);
                    case 1:
                        mTermsOfUseFragment = OnboardingTermsOfUseFragment.newInstance();
                        return mTermsOfUseFragment;
                    case 2:
                        return OnboardingNetworkSelectionFragment.newInstance();
                    case 3:
                        return OnboardingRestoreWalletFragment.newInstance();
                    case 4:
                        return new OnboardingSecurePasswordFragment();
                    case 5:
                        return new OnboardingFingerprintUnlockFragment();
                    case 6:
                        return OnboardingCreatingWalletFragment.newInstance(overridePreviousWallet);
                    case 7:
                        return new OnboardingConfirmationFragment();
                    default:
                        throw new IllegalStateException(
                                String.format(
                                        "No fragment found for ONBOARDING_RESTORE Wallet action at"
                                                + " position %d.",
                                        position));
                }
            }
            case WalletAction.UNLOCK -> {
                return new UnlockWalletFragment();
            }
            case WalletAction.RESTORE -> {
                final boolean overridePreviousWallet = true;
                switch (position) {
                    case 0:
                        return new UnlockWalletFragment();
                    case 1:
                        return OnboardingRestoreWalletFragment.newInstance();
                    case 2:
                        return OnboardingNetworkSelectionFragment.newInstance();
                    case 3:
                        return new OnboardingSecurePasswordFragment();
                    case 4:
                        return new OnboardingFingerprintUnlockFragment();
                    case 5:
                        return OnboardingCreatingWalletFragment.newInstance(overridePreviousWallet);
                    case 6:
                        return new OnboardingConfirmationFragment();
                    default:
                        throw new IllegalStateException(
                                String.format(
                                        "No fragment found for RESTORE Wallet action at position"
                                                + " %d.",
                                        position));
                }
            }
            case WalletAction.BACKUP -> {
                final boolean isOnboarding = false;
                switch (position) {
                    case 0:
                        return OnboardingBackupWalletFragment.newInstance();
                    case 1:
                        return OnboardingRecoveryPhraseFragment.newInstance(isOnboarding);
                    case 2:
                        return OnboardingVerifyRecoveryPhraseFragment.newInstance(
                                isOnboarding, FIRST);
                    case 3:
                        return OnboardingVerifyRecoveryPhraseFragment.newInstance(
                                isOnboarding, SECOND);
                    case 4:
                        return OnboardingVerifyRecoveryPhraseFragment.newInstance(
                                isOnboarding, THIRD);
                    default:
                        throw new IllegalStateException(
                                String.format(
                                        "No fragment found for BACKUP Wallet action at position"
                                                + " %d.",
                                        position));
                }
            }
            default -> throw new IllegalStateException(
                    String.format("No fragment found for Wallet action %s.", mWalletAction));
        }
    }

    @Override
    public int getItemCount() {
        switch (mWalletAction) {
            case WalletAction.ONBOARDING:
                return 1;
            case WalletAction.PASSWORD_CREATION:
                return 11;
            case WalletAction.ONBOARDING_RESTORE:
                return 8;
            case WalletAction.UNLOCK:
                return 1;
            case WalletAction.RESTORE:
                return 7;
            case WalletAction.BACKUP:
                return 5;
            default:
                throw new IllegalStateException(
                        String.format(
                                "Item count not available for Wallet action %s.", mWalletAction));
        }
    }
}
