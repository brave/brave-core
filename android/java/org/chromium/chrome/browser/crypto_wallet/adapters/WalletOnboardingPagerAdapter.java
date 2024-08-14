/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.adapters;

import static org.chromium.chrome.browser.crypto_wallet.fragments.onboarding.OnboardingVerifyRecoveryPhraseFragment.VerificationStep.FIRST;
import static org.chromium.chrome.browser.crypto_wallet.fragments.onboarding.OnboardingVerifyRecoveryPhraseFragment.VerificationStep.SECOND;
import static org.chromium.chrome.browser.crypto_wallet.fragments.onboarding.OnboardingVerifyRecoveryPhraseFragment.VerificationStep.THIRD;

import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentActivity;
import androidx.viewpager2.adapter.FragmentStateAdapter;

import org.chromium.brave_wallet.mojom.BraveWalletP3a;
import org.chromium.brave_wallet.mojom.OnboardingAction;
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

    @NonNull private final BraveWalletP3a mBraveWalletP3A;
    private final boolean mRestartSetupAction;
    private final boolean mRestartRestoreAction;

    @NonNull private WalletAction mWalletAction;

    public WalletOnboardingPagerAdapter(
            @NonNull final FragmentActivity fragmentActivity,
            @NonNull final BraveWalletP3a braveWalletP3a,
            final boolean restartSetupAction,
            final boolean restartRestoreAction) {
        super(fragmentActivity);
        mBraveWalletP3A = braveWalletP3a;
        mRestartSetupAction = restartSetupAction;
        mRestartRestoreAction = restartRestoreAction;
        mWalletAction = WalletAction.UNLOCK;
    }

    public void setWalletAction(@NonNull final WalletAction walletAction) {
        if (walletAction == mWalletAction) {
            return;
        }
        mWalletAction = walletAction;

        if (walletAction == WalletAction.ONBOARDING) {
            mBraveWalletP3A.reportOnboardingAction(OnboardingAction.SHOWN);
        } else if (walletAction == WalletAction.PASSWORD_CREATION) {
            mBraveWalletP3A.reportOnboardingAction(OnboardingAction.LEGAL_AND_PASSWORD);
        } else if (walletAction == WalletAction.ONBOARDING_RESTORE) {
            mBraveWalletP3A.reportOnboardingAction(OnboardingAction.START_RESTORE);
        }

        notifyItemRangeChanged(0, getItemCount());
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
            case ONBOARDING -> {
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
            case PASSWORD_CREATION -> {
                final boolean isOnboarding = true;

                if (position == 0) {
                    return new OnboardingInitWalletFragment(
                            mRestartSetupAction, mRestartRestoreAction);
                } else if (position == 1) {
                    return OnboardingTermsOfUseFragment.newInstance();
                } else if (position == 2) {
                    return OnboardingNetworkSelectionFragment.newInstance();
                } else if (position == 3) {
                    return new OnboardingSecurePasswordFragment();
                } else if (position == 4) {
                    return new OnboardingFingerprintUnlockFragment();
                } else if (position == 5) {
                    return new OnboardingCreatingWalletFragment();
                } else if (position == 6) {
                    return OnboardingRecoveryPhraseFragment.newInstance(isOnboarding);
                } else if (position == 7) {
                    return OnboardingVerifyRecoveryPhraseFragment.newInstance(isOnboarding, FIRST);
                } else if (position == 8) {
                    return OnboardingVerifyRecoveryPhraseFragment.newInstance(isOnboarding, SECOND);
                } else if (position == 9) {
                    return OnboardingVerifyRecoveryPhraseFragment.newInstance(isOnboarding, THIRD);
                } else if (position == 10) {
                    return new OnboardingConfirmationFragment();
                } else {
                    throw new IllegalStateException(
                            String.format(
                                    "No fragment found for PASSWORD_CREATION Wallet action at"
                                            + " position %d.",
                                    position));
                }
            }
            case ONBOARDING_RESTORE -> {
                if (position == 0) {
                    return new OnboardingInitWalletFragment(
                            mRestartSetupAction, mRestartRestoreAction);
                } else if (position == 1) {
                    return OnboardingTermsOfUseFragment.newInstance();
                } else if (position == 2) {
                    return OnboardingNetworkSelectionFragment.newInstance();
                } else if (position == 3) {
                    return OnboardingRestoreWalletFragment.newInstance();
                } else if (position == 4) {
                    return new OnboardingSecurePasswordFragment();
                } else if (position == 5) {
                    return new OnboardingFingerprintUnlockFragment();
                } else if (position == 6) {
                    return new OnboardingCreatingWalletFragment();
                } else if (position == 7) {
                    return new OnboardingConfirmationFragment();
                } else {
                    throw new IllegalStateException(
                            String.format(
                                    "No fragment found for ONBOARDING_RESTORE Wallet action at"
                                            + " position %d.",
                                    position));
                }
            }
            case UNLOCK -> {
                return new UnlockWalletFragment();
            }
            case RESTORE -> {
                if (position == 0) {
                    return new UnlockWalletFragment();
                } else if (position == 1) {
                    return OnboardingRestoreWalletFragment.newInstance();
                } else if (position == 2) {
                    return new OnboardingSecurePasswordFragment();
                } else if (position == 3) {
                    return new OnboardingFingerprintUnlockFragment();
                } else if (position == 4) {
                    return new OnboardingCreatingWalletFragment();
                } else if (position == 5) {
                    return new OnboardingConfirmationFragment();
                } else {
                    throw new IllegalStateException(
                            String.format(
                                    "No fragment found for RESTORE Wallet action at position %d.",
                                    position));
                }
            }
            case BACKUP -> {
                final boolean isOnboarding = false;
                if (position == 0) {
                    return OnboardingBackupWalletFragment.newInstance();
                } else if (position == 1) {
                    return OnboardingRecoveryPhraseFragment.newInstance(isOnboarding);
                } else if (position == 2) {
                    return OnboardingVerifyRecoveryPhraseFragment.newInstance(isOnboarding, FIRST);
                } else if (position == 3) {
                    return OnboardingVerifyRecoveryPhraseFragment.newInstance(isOnboarding, SECOND);
                } else if (position == 4) {
                    return OnboardingVerifyRecoveryPhraseFragment.newInstance(isOnboarding, THIRD);
                } else {
                    throw new IllegalStateException(
                            String.format(
                                    "No fragment found for BACKUP Wallet action at position %d.",
                                    position));
                }
            }
            default -> throw new IllegalStateException(
                    String.format("No fragment found for Wallet action %s.", mWalletAction));
        }
    }

    @Override
    public int getItemCount() {
        if (mWalletAction == WalletAction.ONBOARDING) {
            return 1;
        } else if (mWalletAction == WalletAction.PASSWORD_CREATION) {
            return 11;
        } else if (mWalletAction == WalletAction.ONBOARDING_RESTORE) {
            return 8;
        } else if (mWalletAction == WalletAction.UNLOCK) {
            return 1;
        } else if (mWalletAction == WalletAction.RESTORE) {
            return 6;
        } else if (mWalletAction == WalletAction.BACKUP) {
            return 5;
        }
        throw new IllegalStateException(
                String.format("Item count not available for Wallet action %s.", mWalletAction));
    }
}
