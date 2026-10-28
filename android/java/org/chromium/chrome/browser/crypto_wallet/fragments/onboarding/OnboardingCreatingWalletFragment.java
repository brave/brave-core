/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments.onboarding;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import androidx.appcompat.app.AlertDialog;

import com.google.android.material.dialog.MaterialAlertDialogBuilder;
import com.google.android.material.textfield.TextInputEditText;
import com.google.android.material.textfield.TextInputLayout;

import org.chromium.base.Log;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.RecoverWalletResult;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.domain.KeyringModel;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.ui.text.EmptyTextWatcher;
import org.chromium.ui.widget.Toast;

/** Onboarding fragment for Brave Wallet which shows the spinner while wallet is created/restored */
public class OnboardingCreatingWalletFragment extends BaseOnboardingWalletFragment {

    private static final String TAG = "CreatingWalletFrag";

    private static final int NEXT_PAGE_DELAY_MS = 700;
    private static final String OVERRIDE_PREVIOUS_WALLET_ARG = "overridePreviousWallet";

    private boolean mAddTransitionDelay = true;
    private boolean mOverridePreviousWallet;
    private boolean mCreationHandled;
    // Whether a recovery outcome dialog is on screen. The outcome must not be handled again while
    // the user is answering it.
    private boolean mRecoveryDialogShowing;

    @NonNull
    public static OnboardingCreatingWalletFragment newInstance(
            final boolean overridePreviousWallet) {
        OnboardingCreatingWalletFragment fragment = new OnboardingCreatingWalletFragment();
        Bundle args = new Bundle();
        args.putBoolean(OVERRIDE_PREVIOUS_WALLET_ARG, overridePreviousWallet);
        fragment.setArguments(args);
        return fragment;
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mOverridePreviousWallet =
                requireArguments().getBoolean(OVERRIDE_PREVIOUS_WALLET_ARG, false);
    }

    @Override
    public View onCreateView(
            @NonNull LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_creating_wallet, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        setAnimatedBackground(view.findViewById(R.id.creating_wallet_root));

        // React to the request outcome that lives in the model. LiveData replays the latest value,
        // so a fragment recreated by a rotation is notified even if the request already finished.
        if (mOverridePreviousWallet) {
            mOnboardingViewModel
                    .getWalletRecoveryResult()
                    .observe(getViewLifecycleOwner(), this::onWalletRecoveryResult);
        } else {
            mOnboardingViewModel
                    .getWalletCreationSucceeded()
                    .observe(getViewLifecycleOwner(), this::onWalletCreationSucceeded);
        }
    }

    @Override
    public void onResume() {
        super.onResume();

        PostTask.postDelayedTask(
                TaskTraits.USER_BLOCKING, () -> mAddTransitionDelay = false, NEXT_PAGE_DELAY_MS);

        // Trigger the request; the model runs it only once and keeps it alive across rotations and
        // while the activity is in the background.
        KeyringModel keyringModel = getKeyringModel();
        JsonRpcService jsonRpcService = getJsonRpcService();
        if (keyringModel != null && jsonRpcService != null) {
            mOnboardingViewModel.createOrRestoreWallet(
                    keyringModel, jsonRpcService, getKeyringService(), mOverridePreviousWallet);
        }

        // A recovery dialog is dismissed when the fragment is paused, so re-arm the outcome to
        // show it again instead of leaving the user on the spinner.
        if (mRecoveryDialogShowing) {
            mRecoveryDialogShowing = false;
            mCreationHandled = false;
        }

        // The observer only fires while the fragment is at least started, which can be before it is
        // resumed; handle an outcome that is already available now that it is the resumed page.
        if (mOverridePreviousWallet) {
            onWalletRecoveryResult(mOnboardingViewModel.getWalletRecoveryResult().getValue());
        } else {
            onWalletCreationSucceeded(mOnboardingViewModel.getWalletCreationSucceeded().getValue());
        }
    }

    private void onWalletCreationSucceeded(@Nullable final Boolean succeeded) {
        // Act only while this fragment is the resumed page: the pager keeps it started off screen
        // after we have moved on, and it must not navigate again from there. isResumed() is true
        // only for the current page.
        if (succeeded == null || mCreationHandled || !isResumed()) {
            return;
        }
        mCreationHandled = true;
        if (succeeded) {
            setupWalletModel();
            goToNextPage();
        } else {
            Toast.makeText(requireActivity(), R.string.account_recovery_failed, Toast.LENGTH_LONG)
                    .show();
            requireActivity().finish();
        }
    }

    private void onWalletRecoveryResult(@Nullable final Integer result) {
        if (result == null || mCreationHandled || !isResumed() || mRecoveryDialogShowing) {
            return;
        }
        mCreationHandled = true;
        switch (result) {
            case RecoverWalletResult.SUCCESS -> {
                setupWalletModel();
                goToNextPage();
            }
            case RecoverWalletResult.INVALID_MNEMONIC ->
                    showRecoveryErrorAndReturnToPhrase(R.string.wallet_recovery_invalid_phrase);
            case RecoverWalletResult.MNEMONIC_MISMATCH ->
                    showRecoveryErrorAndReturnToPhrase(R.string.wallet_recovery_phrase_mismatch);
            case RecoverWalletResult.IMPORTED_ACCOUNTS_PRESENT -> showImportedAccountsWarning();
            default -> {
                Toast.makeText(
                                requireActivity(),
                                R.string.account_recovery_failed,
                                Toast.LENGTH_LONG)
                        .show();
                returnToPhrasePage();
            }
        }
    }

    private void showRecoveryErrorAndReturnToPhrase(@StringRes final int messageResId) {
        mRecoveryDialogShowing = true;
        showDialog(
                new MaterialAlertDialogBuilder(
                                requireContext(), R.style.BraveWalletAlertDialogTheme)
                        .setMessage(messageResId)
                        .setPositiveButton(
                                R.string.ok,
                                (dialog, which) -> {
                                    mRecoveryDialogShowing = false;
                                    returnToPhrasePage();
                                })
                        .setCancelable(false)
                        .create());
    }

    private void showImportedAccountsWarning() {
        mRecoveryDialogShowing = true;
        final String confirmationPhrase =
                getString(R.string.wallet_recovery_imported_accounts_confirmation_phrase);
        final View container =
                LayoutInflater.from(requireContext())
                        .inflate(R.layout.dialog_wallet_recovery_imported_accounts, null);
        final TextView confirmationLabel =
                container.findViewById(R.id.imported_accounts_confirmation_label);
        confirmationLabel.setText(
                getString(
                        R.string.wallet_recovery_imported_accounts_confirmation,
                        confirmationPhrase));
        final TextInputLayout inputLayout =
                container.findViewById(R.id.imported_accounts_confirmation_layout);
        inputLayout.setHint(confirmationPhrase);
        final TextInputEditText input =
                container.findViewById(R.id.imported_accounts_confirmation_input);

        final AlertDialog alertDialog =
                new MaterialAlertDialogBuilder(
                                requireContext(), R.style.BraveWalletAlertDialogTheme)
                        .setTitle(R.string.wallet_recovery_imported_accounts_warning_title)
                        .setMessage(R.string.wallet_recovery_imported_accounts_warning_description)
                        .setView(container)
                        .setPositiveButton(
                                R.string.wallet_recovery_imported_accounts_warning_confirm, null)
                        .setNegativeButton(R.string.cancel, null)
                        .setCancelable(false)
                        .create();
        alertDialog.setOnShowListener(
                dialog -> {
                    final Button okButton = alertDialog.getButton(AlertDialog.BUTTON_POSITIVE);
                    final Button cancelButton = alertDialog.getButton(AlertDialog.BUTTON_NEGATIVE);
                    okButton.setEnabled(false);
                    okButton.setOnClickListener(
                            v -> {
                                if (!matchesConfirmationPhrase(
                                        input.getText(), confirmationPhrase)) {
                                    return;
                                }
                                mRecoveryDialogShowing = false;
                                alertDialog.dismiss();
                                retryRecoveryAllowingImportedAccountRemoval();
                            });
                    cancelButton.setOnClickListener(
                            v -> {
                                mRecoveryDialogShowing = false;
                                alertDialog.dismiss();
                                returnToPhrasePage();
                            });
                    input.addTextChangedListener(
                            new EmptyTextWatcher() {
                                @Override
                                public void onTextChanged(
                                        CharSequence s, int start, int before, int count) {
                                    okButton.setEnabled(
                                            matchesConfirmationPhrase(s, confirmationPhrase));
                                }
                            });
                });
        showDialog(alertDialog);
    }

    /** Compares the typed text with the confirmation phrase, ignoring padding and letter case. */
    private static boolean matchesConfirmationPhrase(
            @Nullable final CharSequence typed, @NonNull final String confirmationPhrase) {
        return typed != null && typed.toString().trim().equalsIgnoreCase(confirmationPhrase);
    }

    private void retryRecoveryAllowingImportedAccountRemoval() {
        mCreationHandled = false;
        mOnboardingViewModel.prepareWalletCreationRetry();
        KeyringModel keyringModel = getKeyringModel();
        JsonRpcService jsonRpcService = getJsonRpcService();
        if (keyringModel == null || jsonRpcService == null) {
            returnToPhrasePage();
            return;
        }
        mOnboardingViewModel.createOrRestoreWallet(
                keyringModel,
                jsonRpcService,
                getKeyringService(),
                /* overridePreviousWallet= */ true,
                /* allowImportedAccountRemoval= */ true);
    }

    private void returnToPhrasePage() {
        mCreationHandled = false;
        mOnboardingViewModel.prepareWalletCreationRetry();
        if (mOnNextPage != null) {
            mOnNextPage.showRestorePhrasePage();
        }
    }

    private void setupWalletModel() {
        try {
            BraveActivity activity = BraveActivity.getBraveActivity();
            activity.setupWalletModel();
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "setupWalletModel", e);
        }
    }

    private void goToNextPage() {
        // Go to the next page after wallet creation is done.
        if (mOnNextPage == null) {
            return;
        }
        // Unlock-screen recovery places fingerprint after recovery. Skip it when biometrics are
        // unavailable.
        final int pages =
                mOverridePreviousWallet && !Utils.isBiometricSupported(requireContext()) ? 2 : 1;
        // Add small delay if the Wallet creation completes faster than {@code NEXT_PAGE_DELAY_MS}.
        if (mAddTransitionDelay) {
            PostTask.postDelayedTask(
                    TaskTraits.USER_BLOCKING,
                    () -> {
                        // The fragment may be detached by the time the delayed task runs.
                        if (mOnNextPage != null) {
                            mOnNextPage.incrementPages(pages);
                        }
                    },
                    NEXT_PAGE_DELAY_MS);
        } else {
            mOnNextPage.incrementPages(pages);
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
