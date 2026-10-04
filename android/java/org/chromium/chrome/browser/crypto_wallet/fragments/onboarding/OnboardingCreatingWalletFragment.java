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

import org.chromium.base.Log;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.domain.KeyringModel;
import org.chromium.ui.widget.Toast;

/** Onboarding fragment for Brave Wallet which shows the spinner while wallet is created/restored */
public class OnboardingCreatingWalletFragment extends BaseOnboardingWalletFragment {

    private static final String TAG = "CreatingWalletFrag";

    private static final int NEXT_PAGE_DELAY_MS = 700;
    private static final String OVERRIDE_PREVIOUS_WALLET_ARG = "overridePreviousWallet";

    private boolean mAddTransitionDelay = true;
    private boolean mOverridePreviousWallet;
    private boolean mCreationHandled;

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
        mOnboardingViewModel
                .getWalletCreationSucceeded()
                .observe(getViewLifecycleOwner(), this::onWalletCreationSucceeded);
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

        // The observer only fires while the fragment is at least started, which can be before it is
        // resumed; handle an outcome that is already available now that it is the resumed page.
        onWalletCreationSucceeded(mOnboardingViewModel.getWalletCreationSucceeded().getValue());
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
        // Add small delay if the Wallet creation completes faster than {@code NEXT_PAGE_DELAY_MS}.
        if (mAddTransitionDelay) {
            PostTask.postDelayedTask(
                    TaskTraits.USER_BLOCKING,
                    () -> {
                        // The fragment may be detached by the time the delayed task runs.
                        if (mOnNextPage != null) {
                            mOnNextPage.incrementPages(1);
                        }
                    },
                    NEXT_PAGE_DELAY_MS);
        } else {
            mOnNextPage.incrementPages(1);
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
