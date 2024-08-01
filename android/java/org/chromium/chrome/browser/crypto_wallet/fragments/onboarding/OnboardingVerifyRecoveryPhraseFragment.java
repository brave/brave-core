/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments.onboarding;

import android.annotation.SuppressLint;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.recyclerview.widget.GridLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.brave_wallet.mojom.BraveWalletP3a;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.brave_wallet.mojom.OnboardingAction;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.adapters.RecoveryPhraseAdapter;
import org.chromium.chrome.browser.crypto_wallet.util.ItemOffsetDecoration;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.ui.widget.Toast;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class OnboardingVerifyRecoveryPhraseFragment extends BaseOnboardingWalletFragment {
    private static final String IS_ONBOARDING_ARG = "isOnboarding";

    private RecyclerView mRecoveryPhrasesRecyclerView;
    private RecyclerView mSelectedPhraseRecyclerView;

    private RecoveryPhraseAdapter mRecoveryPhrasesAdapter;
    private RecoveryPhraseAdapter mRecoveryPhrasesToVerifyAdapter;

    private Button mRecoveryPhraseButton;
    private List<String> mRecoveryPhrases;
    private boolean mIsOnboarding;

    public interface OnRecoveryPhraseSelected {
        void onSelectedRecoveryPhrase(String phrase);
    }

    @NonNull
    public static OnboardingVerifyRecoveryPhraseFragment newInstance(boolean isOnboarding) {
        OnboardingVerifyRecoveryPhraseFragment fragment = new OnboardingVerifyRecoveryPhraseFragment();

        Bundle args = new Bundle();
        args.putBoolean(IS_ONBOARDING_ARG, isOnboarding);
        fragment.setArguments(args);

        return fragment;
    }

    @Override
    public View onCreateView(
           @NonNull LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        mIsOnboarding = requireArguments().getBoolean(IS_ONBOARDING_ARG, false);
        return inflater.inflate(R.layout.fragment_verify_recovery_phrase, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        mRecoveryPhraseButton = view.findViewById(R.id.btn_verify_recovery_phrase_continue);
        mRecoveryPhraseButton.setOnClickListener(
                v -> {
                    if (mRecoveryPhrasesToVerifyAdapter != null
                            && mRecoveryPhrasesToVerifyAdapter.getRecoveryPhraseList().size() > 0) {
                        KeyringService keyringService = getKeyringService();
                        BraveWalletP3a braveWalletP3A = getBraveWalletP3A();
                        if (keyringService != null) {
                            keyringService.getWalletMnemonic(
                                    mOnboardingViewModel.getPassword(),
                                    result -> {
                                        String recoveryPhraseToVerify =
                                                Utils.getRecoveryPhraseFromList(
                                                        mRecoveryPhrasesToVerifyAdapter
                                                                .getRecoveryPhraseList());
                                        if (result.equals(recoveryPhraseToVerify)) {
                                            if (braveWalletP3A != null && mIsOnboarding) {
                                                braveWalletP3A.reportOnboardingAction(
                                                        OnboardingAction.COMPLETE);
                                            }
                                            keyringService.notifyWalletBackupComplete();
                                            if (mOnNextPage != null) {
                                                if (mIsOnboarding) {
                                                    // Show confirmation screen
                                                    // only during onboarding process.
                                                    mOnNextPage.incrementPages(1);
                                                } else {
                                                    // It's important to call the
                                                    // `showWallet` method instead of
                                                    // finishing the activity as we need to
                                                    // reload the portfolio section to hide the
                                                    // backup banner.
                                                    mOnNextPage.showWallet(true);
                                                }
                                            }
                                        } else {
                                            phraseNotMatch();
                                        }
                                    });
                        } else {
                            phraseNotMatch();
                        }
                    } else {
                        phraseNotMatch();
                    }
                });
        TextView recoveryPhraseSkipButton = view.findViewById(R.id.btn_verify_recovery_phrase_skip);
        recoveryPhraseSkipButton.setOnClickListener(
                v -> {
                    BraveWalletP3a braveWalletP3A = getBraveWalletP3A();
                    if (braveWalletP3A != null && mIsOnboarding) {
                        braveWalletP3A.reportOnboardingAction(
                                OnboardingAction.COMPLETE_RECOVERY_SKIPPED);
                    }
                    if (mIsOnboarding) {
                        if (mOnNextPage != null) {
                            // Show confirmation screen
                            // only during onboarding process.
                            mOnNextPage.incrementPages(1);
                        }
                    } else {
                        requireActivity().finish();
                    }
                });

        KeyringService keyringService = getKeyringService();
        if (keyringService != null) {
            keyringService.getWalletMnemonic(
                    mOnboardingViewModel.getPassword(),
                    result -> {
                        mRecoveryPhrases = Utils.getRecoveryPhraseAsList(result);
                        Collections.shuffle(mRecoveryPhrases);
                        setupRecoveryPhraseRecyclerView(view);
                        setupSelectedRecoveryPhraseRecyclerView(view);
                    });
        }
    }

    private void phraseNotMatch() {
        resetRecoveryPhrasesViews();
        Toast.makeText(requireActivity(), R.string.phrases_did_not_match, Toast.LENGTH_SHORT).show();
    }

    @SuppressLint("NotifyDataSetChanged")
    private void resetRecoveryPhrasesViews() {
        if (mRecoveryPhrasesAdapter != null && mRecoveryPhrasesRecyclerView != null) {
            mRecoveryPhrasesAdapter = new RecoveryPhraseAdapter();
            mRecoveryPhrasesAdapter.setRecoveryPhraseList(mRecoveryPhrases);
            mRecoveryPhrasesAdapter.setOnRecoveryPhraseSelectedListener(mOnRecoveryPhraseSelected);
            mRecoveryPhrasesRecyclerView.setAdapter(mRecoveryPhrasesAdapter);
        }
        if (mRecoveryPhrasesToVerifyAdapter != null) {
            mRecoveryPhrasesToVerifyAdapter.setRecoveryPhraseList(new ArrayList<>());
            mRecoveryPhrasesToVerifyAdapter.notifyDataSetChanged();
        }
    }

    private void setupRecoveryPhraseRecyclerView(@NonNull View view) {
        mRecoveryPhrasesRecyclerView = view.findViewById(R.id.recovery_phrase_recyclerview);
        assert getActivity() != null;
        mRecoveryPhrasesRecyclerView.addItemDecoration(
                new ItemOffsetDecoration(getActivity(), R.dimen.zero_margin));
        GridLayoutManager layoutManager = new GridLayoutManager(getActivity(), 3);
        mRecoveryPhrasesRecyclerView.setLayoutManager(layoutManager);

        mRecoveryPhrasesAdapter = new RecoveryPhraseAdapter();
        mRecoveryPhrasesAdapter.setRecoveryPhraseList(mRecoveryPhrases);
        mRecoveryPhrasesAdapter.setOnRecoveryPhraseSelectedListener(mOnRecoveryPhraseSelected);
        mRecoveryPhrasesRecyclerView.setAdapter(mRecoveryPhrasesAdapter);
    }

    private void setupSelectedRecoveryPhraseRecyclerView(@NonNull View view) {
        mSelectedPhraseRecyclerView = view.findViewById(R.id.recovery_phrase_selected_recyclerview);
        assert getActivity() != null;
        mSelectedPhraseRecyclerView.addItemDecoration(
                new ItemOffsetDecoration(getActivity(), R.dimen.zero_margin));
        GridLayoutManager layoutManager = new GridLayoutManager(getActivity(), 3);
        mSelectedPhraseRecyclerView.setLayoutManager(layoutManager);

        mRecoveryPhrasesToVerifyAdapter = new RecoveryPhraseAdapter();
        mRecoveryPhrasesToVerifyAdapter.setRecoveryPhraseList(
                mRecoveryPhrasesAdapter.getSelectedRecoveryPhraseList());
        mRecoveryPhrasesToVerifyAdapter.setOnRecoveryPhraseSelectedListener(
                mOnSelectedRecoveryPhraseSelected);
        mRecoveryPhrasesToVerifyAdapter.setSelectedRecoveryPhrase(true);
        mSelectedPhraseRecyclerView.setAdapter(mRecoveryPhrasesToVerifyAdapter);
    }

    OnRecoveryPhraseSelected mOnRecoveryPhraseSelected =
            new OnRecoveryPhraseSelected() {
                @Override
                @SuppressLint("NotifyDataSetChanged")
                public void onSelectedRecoveryPhrase(String phrase) {
                    if (mRecoveryPhrasesAdapter != null) {
                        mRecoveryPhrasesAdapter.notifyDataSetChanged();
                    }

                    if (mRecoveryPhrasesAdapter != null
                            && mRecoveryPhrasesToVerifyAdapter != null) {
                        List<String> newList =
                                new ArrayList<>(
                                        mRecoveryPhrasesAdapter.getSelectedRecoveryPhraseList());
                        mRecoveryPhrasesToVerifyAdapter.setRecoveryPhraseList(newList);
                        mRecoveryPhrasesToVerifyAdapter.notifyDataSetChanged();
                    }

                    if (mRecoveryPhrasesAdapter != null
                            && mRecoveryPhrasesAdapter.getSelectedRecoveryPhraseList().size()
                                    == mRecoveryPhrases.size()) {
                        mRecoveryPhraseButton.setAlpha(1f);
                        mRecoveryPhraseButton.setEnabled(true);
                    }
                }
            };

    OnRecoveryPhraseSelected mOnSelectedRecoveryPhraseSelected =
            new OnRecoveryPhraseSelected() {
                @Override
                @SuppressLint("NotifyDataSetChanged")
                public void onSelectedRecoveryPhrase(String phrase) {
                    if (mRecoveryPhrasesAdapter != null) {
                        mRecoveryPhrasesAdapter.addPhraseAtPosition(
                                mRecoveryPhrases.indexOf(phrase), phrase);
                        mRecoveryPhrasesAdapter.removeSelectedPhrase(phrase);
                        mRecoveryPhrasesAdapter.notifyDataSetChanged();
                    }

                    if (mRecoveryPhrasesAdapter != null
                            && mRecoveryPhrasesToVerifyAdapter != null) {
                        mRecoveryPhrasesToVerifyAdapter.setRecoveryPhraseList(
                                mRecoveryPhrasesAdapter.getSelectedRecoveryPhraseList());
                        mRecoveryPhrasesToVerifyAdapter.notifyDataSetChanged();
                    }

                    if (mRecoveryPhrasesAdapter != null
                            && mRecoveryPhrasesAdapter.getSelectedRecoveryPhraseList().size()
                                    == mRecoveryPhrases.size()) {
                        mRecoveryPhraseButton.setAlpha(1f);
                        mRecoveryPhraseButton.setEnabled(true);
                    }
                }
            };
}
