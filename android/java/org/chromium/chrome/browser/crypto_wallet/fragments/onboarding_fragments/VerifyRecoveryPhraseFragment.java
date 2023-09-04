/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments.onboarding_fragments;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.lifecycle.ViewModelProvider;
import androidx.lifecycle.ViewModelStoreOwner;
import androidx.recyclerview.widget.GridLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.brave_wallet.mojom.BraveWalletP3a;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.brave_wallet.mojom.OnboardingAction;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletActivity;
import org.chromium.chrome.browser.crypto_wallet.adapters.RecoveryPhraseAdapter;
import org.chromium.chrome.browser.crypto_wallet.model.OnboardingViewModel;
import org.chromium.chrome.browser.crypto_wallet.util.ItemOffsetDecoration;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.ui.widget.Toast;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class VerifyRecoveryPhraseFragment extends CryptoOnboardingFragment {
    private static final String IS_ONBOARDING_ARG = "isOnboarding";

    private RecyclerView recoveryPhrasesRecyclerView;
    private RecyclerView selectedPhraseRecyclerView;

    private RecoveryPhraseAdapter recoveryPhrasesAdapter;
    private RecoveryPhraseAdapter recoveryPhrasesToVerifyAdapter;

    private Button recoveryPhraseButton;
    private List<String> recoveryPhrases;
    private boolean mIsOnboarding;
    private OnboardingViewModel mOnboardingViewModel;

    private KeyringService getKeyringService() {
        Activity activity = getActivity();
        if (activity instanceof BraveWalletActivity) {
            return ((BraveWalletActivity) activity).getKeyringService();
        }

        return null;
    }

    private BraveWalletP3a getBraveWalletP3A() {
        Activity activity = getActivity();
        if (activity instanceof BraveWalletActivity) {
            return ((BraveWalletActivity) activity).getBraveWalletP3A();
        }

        return null;
    }

    public interface OnRecoveryPhraseSelected {
        void onSelectedRecoveryPhrase(String phrase);
    }

    public static VerifyRecoveryPhraseFragment newInstance(boolean isOnboarding) {
        VerifyRecoveryPhraseFragment fragment = new VerifyRecoveryPhraseFragment();

        Bundle args = new Bundle();
        args.putBoolean(IS_ONBOARDING_ARG, isOnboarding);
        fragment.setArguments(args);

        return fragment;
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        mIsOnboarding = getArguments().getBoolean(IS_ONBOARDING_ARG, false);
        return inflater.inflate(R.layout.fragment_verify_recovery_phrase, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        mOnboardingViewModel = new ViewModelProvider((ViewModelStoreOwner) requireActivity())
                                       .get(OnboardingViewModel.class);
        recoveryPhraseButton = view.findViewById(R.id.btn_verify_recovery_phrase_continue);
        recoveryPhraseButton.setOnClickListener(v -> {
            String password = mOnboardingViewModel.getPassword().getValue();
            if (recoveryPhrasesToVerifyAdapter != null
                    && recoveryPhrasesToVerifyAdapter.getRecoveryPhraseList().size() > 0
                    && password != null) {
                KeyringService keyringService = getKeyringService();
                BraveWalletP3a braveWalletP3A = getBraveWalletP3A();
                if (keyringService != null) {
                    keyringService.getMnemonicForDefaultKeyring(password, result -> {
                        String recoveryPhraseToVerify = Utils.getRecoveryPhraseFromList(
                                recoveryPhrasesToVerifyAdapter.getRecoveryPhraseList());
                        if (result.equals(recoveryPhraseToVerify)) {
                            if (braveWalletP3A != null && mIsOnboarding) {
                                braveWalletP3A.reportOnboardingAction(OnboardingAction.COMPLETE);
                            }
                            keyringService.notifyWalletBackupComplete();
                            onNextPage.gotoNextPage(true);
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
        recoveryPhraseSkipButton.setOnClickListener(v -> {
            BraveWalletP3a braveWalletP3A = getBraveWalletP3A();
            if (braveWalletP3A != null && mIsOnboarding) {
                braveWalletP3A.reportOnboardingAction(OnboardingAction.COMPLETE_RECOVERY_SKIPPED);
            }
            onNextPage.gotoNextPage(true);
        });

        mOnboardingViewModel.getPassword().observe(getViewLifecycleOwner(), password -> {
            if (password == null) {
                return;
            }
            KeyringService keyringService = getKeyringService();
            if (keyringService != null) {
                keyringService.getMnemonicForDefaultKeyring(password, result -> {
                    recoveryPhrases = Utils.getRecoveryPhraseAsList(result);
                    Collections.shuffle(recoveryPhrases);
                    setupRecoveryPhraseRecyclerView(view);
                    setupSelectedRecoveryPhraseRecyclerView(view);
                });
            }
        });
    }

    private void phraseNotMatch() {
        resetRecoveryPhrasesViews();
        assert getActivity() != null;
        Toast.makeText(getActivity(), R.string.phrases_did_not_match, Toast.LENGTH_SHORT).show();
    }

    @SuppressLint("NotifyDataSetChanged")
    private void resetRecoveryPhrasesViews() {
        if (recoveryPhrasesAdapter != null && recoveryPhrasesRecyclerView != null) {
            recoveryPhrasesAdapter = new RecoveryPhraseAdapter();
            recoveryPhrasesAdapter.setRecoveryPhraseList(recoveryPhrases);
            recoveryPhrasesAdapter.setOnRecoveryPhraseSelectedListener(onRecoveryPhraseSelected);
            recoveryPhrasesRecyclerView.setAdapter(recoveryPhrasesAdapter);
        }
        if (recoveryPhrasesToVerifyAdapter != null) {
            recoveryPhrasesToVerifyAdapter.setRecoveryPhraseList(new ArrayList<>());
            recoveryPhrasesToVerifyAdapter.notifyDataSetChanged();
        }
    }

    private void setupRecoveryPhraseRecyclerView(View view) {
        recoveryPhrasesRecyclerView = view.findViewById(R.id.recovery_phrase_recyclerview);
        assert getActivity() != null;
        recoveryPhrasesRecyclerView.addItemDecoration(
                new ItemOffsetDecoration(getActivity(), R.dimen.zero_margin));
        GridLayoutManager layoutManager = new GridLayoutManager(getActivity(), 3);
        recoveryPhrasesRecyclerView.setLayoutManager(layoutManager);

        recoveryPhrasesAdapter = new RecoveryPhraseAdapter();
        recoveryPhrasesAdapter.setRecoveryPhraseList(recoveryPhrases);
        recoveryPhrasesAdapter.setOnRecoveryPhraseSelectedListener(onRecoveryPhraseSelected);
        recoveryPhrasesRecyclerView.setAdapter(recoveryPhrasesAdapter);
    }

    private void setupSelectedRecoveryPhraseRecyclerView(View view) {
        selectedPhraseRecyclerView = view.findViewById(R.id.recovery_phrase_selected_recyclerview);
        assert getActivity() != null;
        selectedPhraseRecyclerView.addItemDecoration(
                new ItemOffsetDecoration(getActivity(), R.dimen.zero_margin));
        GridLayoutManager layoutManager = new GridLayoutManager(getActivity(), 3);
        selectedPhraseRecyclerView.setLayoutManager(layoutManager);

        recoveryPhrasesToVerifyAdapter = new RecoveryPhraseAdapter();
        recoveryPhrasesToVerifyAdapter.setRecoveryPhraseList(
                recoveryPhrasesAdapter.getSelectedRecoveryPhraseList());
        recoveryPhrasesToVerifyAdapter.setOnRecoveryPhraseSelectedListener(
                onSelectedRecoveryPhraseSelected);
        recoveryPhrasesToVerifyAdapter.setSelectedRecoveryPhrase(true);
        selectedPhraseRecyclerView.setAdapter(recoveryPhrasesToVerifyAdapter);
    }

    OnRecoveryPhraseSelected onRecoveryPhraseSelected = new OnRecoveryPhraseSelected() {
        @Override
        @SuppressLint("NotifyDataSetChanged")
        public void onSelectedRecoveryPhrase(String phrase) {
            if (recoveryPhrasesAdapter != null) {
                recoveryPhrasesAdapter.notifyDataSetChanged();
            }

            if (recoveryPhrasesAdapter != null && recoveryPhrasesToVerifyAdapter != null) {
                List<String> newList =
                        new ArrayList<>(recoveryPhrasesAdapter.getSelectedRecoveryPhraseList());
                recoveryPhrasesToVerifyAdapter.setRecoveryPhraseList(newList);
                recoveryPhrasesToVerifyAdapter.notifyDataSetChanged();
            }

            if (recoveryPhrasesAdapter != null
                    && recoveryPhrasesAdapter.getSelectedRecoveryPhraseList().size()
                            == recoveryPhrases.size()) {
                recoveryPhraseButton.setAlpha(1f);
                recoveryPhraseButton.setEnabled(true);
            }
        }
    };

    OnRecoveryPhraseSelected onSelectedRecoveryPhraseSelected = new OnRecoveryPhraseSelected() {
        @Override
        @SuppressLint("NotifyDataSetChanged")
        public void onSelectedRecoveryPhrase(String phrase) {
            if (recoveryPhrasesAdapter != null) {
                recoveryPhrasesAdapter.addPhraseAtPosition(recoveryPhrases.indexOf(phrase), phrase);
                recoveryPhrasesAdapter.removeSelectedPhrase(phrase);
                recoveryPhrasesAdapter.notifyDataSetChanged();
            }

            if (recoveryPhrasesAdapter != null && recoveryPhrasesToVerifyAdapter != null) {
                recoveryPhrasesToVerifyAdapter.setRecoveryPhraseList(
                        recoveryPhrasesAdapter.getSelectedRecoveryPhraseList());
                recoveryPhrasesToVerifyAdapter.notifyDataSetChanged();
            }

            if (recoveryPhrasesAdapter != null
                    && recoveryPhrasesAdapter.getSelectedRecoveryPhraseList().size()
                            == recoveryPhrases.size()) {
                recoveryPhraseButton.setAlpha(1f);
                recoveryPhraseButton.setEnabled(true);
            }
        }
    };
}
