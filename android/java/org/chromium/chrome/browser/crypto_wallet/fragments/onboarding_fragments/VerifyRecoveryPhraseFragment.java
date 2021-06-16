/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments.onboarding_fragments;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.FragmentActivity;
import androidx.recyclerview.widget.GridLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.adapters.RecoveryPhraseAdapter;
import org.chromium.chrome.browser.crypto_wallet.fragments.AddAccountOnboardingDialogFragment;
import org.chromium.chrome.browser.crypto_wallet.util.ItemOffsetDecoration;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class VerifyRecoveryPhraseFragment extends CryptoOnboardingFragment {
    private RecoveryPhraseAdapter recoveryPhraseAdapter;
    private RecyclerView phraseRecyclerView;
    private RecyclerView selectedPhraseRecyclerView;
    private RecoveryPhraseAdapter selectedRecoveryPhraseAdapter;
    private Button recoveryPhraseButton;
    private List<String> recoveryPhrases;

    public interface OnRecoveryPhraseSelected {
        void onSelectedRecoveryPhrase();
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        recoveryPhrases = new ArrayList<String>(Arrays.asList(Utils.recoveryPhrase.split(" ")));
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_verify_recovery_phrase, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        recoveryPhraseButton = view.findViewById(R.id.btn_verify_recovery_phrase_continue);
        recoveryPhraseButton.setOnClickListener(v -> {
            onNextPage.gotoNextPage(true);
            showAddAccountDialog();
        });
        TextView recoveryPhraseSkipButton = view.findViewById(R.id.btn_verify_recovery_phrase_skip);
        recoveryPhraseSkipButton.setOnClickListener(v -> onNextPage.gotoNextPage(true));

        setupRecoveryPhraseRecyclerView(view);
        setupSelectedRecoveryPhraseRecyclerView(view);
    }

    private void setupRecoveryPhraseRecyclerView(View view) {
        phraseRecyclerView = view.findViewById(R.id.recovery_phrase_recyclerview);
        phraseRecyclerView.setHasFixedSize(true);
        assert getActivity() != null;
        phraseRecyclerView.addItemDecoration(
                new ItemOffsetDecoration(getActivity(), R.dimen.zero_margin));
        GridLayoutManager layoutManager = new GridLayoutManager(getActivity(), 3);
        phraseRecyclerView.setLayoutManager(layoutManager);

        recoveryPhraseAdapter = new RecoveryPhraseAdapter();
        recoveryPhraseAdapter.setRecoveryPhraseMap(Utils.getRecoveryPhraseMap(recoveryPhrases));
        recoveryPhraseAdapter.setOnRecoveryPhraseSelectedListener(onRecoveryPhraseSelected);
        phraseRecyclerView.setAdapter(recoveryPhraseAdapter);
    }

    private void setupSelectedRecoveryPhraseRecyclerView(View view) {
        selectedPhraseRecyclerView = view.findViewById(R.id.recovery_phrase_selected_recyclerview);
        selectedPhraseRecyclerView.setHasFixedSize(true);
        assert getActivity() != null;
        selectedPhraseRecyclerView.addItemDecoration(
                new ItemOffsetDecoration(getActivity(), R.dimen.zero_margin));
        GridLayoutManager layoutManager = new GridLayoutManager(getActivity(), 3);
        selectedPhraseRecyclerView.setLayoutManager(layoutManager);

        selectedRecoveryPhraseAdapter = new RecoveryPhraseAdapter();
        selectedRecoveryPhraseAdapter.setRecoveryPhraseMap(
                recoveryPhraseAdapter.getSelectedRecoveryPhraseMap());
        selectedPhraseRecyclerView.setAdapter(selectedRecoveryPhraseAdapter);
    }

    private void showAddAccountDialog() {
        AddAccountOnboardingDialogFragment addAccountOnboardingDialogFragment =
                new AddAccountOnboardingDialogFragment();
        addAccountOnboardingDialogFragment.setCancelable(false);
        assert getActivity() != null;
        addAccountOnboardingDialogFragment.show(
                ((FragmentActivity) getActivity()).getSupportFragmentManager(),
                "AddAccountOnboardingDialog");
    }

    OnRecoveryPhraseSelected onRecoveryPhraseSelected = new OnRecoveryPhraseSelected() {
        @Override
        public void onSelectedRecoveryPhrase() {
            if (phraseRecyclerView != null && recoveryPhraseAdapter != null) {
                recoveryPhraseAdapter.notifyDataSetChanged();
            }

            if (selectedPhraseRecyclerView != null && recoveryPhraseAdapter != null
                    && selectedRecoveryPhraseAdapter != null) {
                selectedRecoveryPhraseAdapter.setRecoveryPhraseMap(
                        recoveryPhraseAdapter.getSelectedRecoveryPhraseMap());
                selectedRecoveryPhraseAdapter.notifyDataSetChanged();
            }

            if (recoveryPhraseAdapter != null
                    && recoveryPhraseAdapter.getSelectedRecoveryPhraseMap().size()
                            == recoveryPhrases.size()) {
                recoveryPhraseButton.setAlpha(1f);
                recoveryPhraseButton.setEnabled(true);
            }
        }
    };
}
