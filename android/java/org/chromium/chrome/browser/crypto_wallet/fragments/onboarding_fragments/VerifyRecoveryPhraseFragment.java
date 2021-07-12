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

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.BraveWalletNativeWorker;
import org.chromium.chrome.browser.crypto_wallet.adapters.RecoveryPhraseAdapter;
import org.chromium.chrome.browser.crypto_wallet.fragments.AddAccountOnboardingDialogFragment;
import org.chromium.chrome.browser.crypto_wallet.util.ItemOffsetDecoration;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.ui.widget.Toast;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

public class VerifyRecoveryPhraseFragment extends CryptoOnboardingFragment {
    private RecyclerView recoveryPhrasesRecyclerView;
    private RecyclerView selectedPhraseRecyclerView;

    private RecoveryPhraseAdapter recoveryPhrasesAdapter;
    private RecoveryPhraseAdapter recoveryPhrasesToVerifyAdapter;

    private Button recoveryPhraseButton;
    private List<String> recoveryPhrases;

    public interface OnRecoveryPhraseSelected {
        void onSelectedRecoveryPhrase(String phrase);
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_verify_recovery_phrase, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        recoveryPhrases = Utils.getRecoveryPhraseAsList();
        Log.e("NTP", "recoveryPhrases : " + Utils.getRecoveryPhraseFromList(recoveryPhrases));
        Collections.shuffle(recoveryPhrases);
        recoveryPhraseButton = view.findViewById(R.id.btn_verify_recovery_phrase_continue);
        recoveryPhraseButton.setOnClickListener(v -> {
            if (isRecoveryPhraseVerified()) {
                onNextPage.gotoNextPage(true);
                showAddAccountDialog();
            } else {
                resetRecoveryPhrasesViews();
                assert getActivity() != null;
                Toast.makeText(getActivity(), R.string.phrases_did_not_match, Toast.LENGTH_SHORT)
                        .show();
            }
        });
        TextView recoveryPhraseSkipButton = view.findViewById(R.id.btn_verify_recovery_phrase_skip);
        recoveryPhraseSkipButton.setOnClickListener(v -> onNextPage.gotoNextPage(true));

        setupRecoveryPhraseRecyclerView(view);
        setupSelectedRecoveryPhraseRecyclerView(view);
    }

    private boolean isRecoveryPhraseVerified() {
        if (recoveryPhrasesToVerifyAdapter != null
                && recoveryPhrasesToVerifyAdapter.getRecoveryPhraseMap().size() > 0) {
            String recoveryPhraseToVerify = Utils.getRecoveryPhraseFromList(
                    recoveryPhrasesToVerifyAdapter.getRecoveryPhraseMap());
            Log.e("NTP",
                    "BraveWalletNativeWorker.getInstance().getRecoveryWords() : "
                            + BraveWalletNativeWorker.getInstance().getRecoveryWords());
            Log.e("NTP", "recoveryPhraseToVerify : " + recoveryPhraseToVerify);
            return BraveWalletNativeWorker.getInstance().getRecoveryWords().equals(
                    recoveryPhraseToVerify);
        }
        return false;
    }

    private void resetRecoveryPhrasesViews() {
        if (recoveryPhrasesAdapter != null && recoveryPhrasesRecyclerView != null) {
            recoveryPhrasesAdapter = new RecoveryPhraseAdapter();
            recoveryPhrasesAdapter.setRecoveryPhraseMap(recoveryPhrases);
            recoveryPhrasesAdapter.setOnRecoveryPhraseSelectedListener(onRecoveryPhraseSelected);
            recoveryPhrasesRecyclerView.setAdapter(recoveryPhrasesAdapter);
        }
        if (recoveryPhrasesToVerifyAdapter != null) {
            recoveryPhrasesToVerifyAdapter.setRecoveryPhraseMap(new ArrayList<>());
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
        recoveryPhrasesAdapter.setRecoveryPhraseMap(recoveryPhrases);
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
        recoveryPhrasesToVerifyAdapter.setRecoveryPhraseMap(
                recoveryPhrasesAdapter.getSelectedRecoveryPhraseMap());
        recoveryPhrasesToVerifyAdapter.setOnRecoveryPhraseSelectedListener(
                onSelectedRecoveryPhraseSelected);
        recoveryPhrasesToVerifyAdapter.setSelectedRecoveryPhrase(true);
        selectedPhraseRecyclerView.setAdapter(recoveryPhrasesToVerifyAdapter);
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
        public void onSelectedRecoveryPhrase(String phrase) {
            if (recoveryPhrasesAdapter != null) {
                recoveryPhrasesAdapter.notifyDataSetChanged();
            }

            if (recoveryPhrasesAdapter != null && recoveryPhrasesToVerifyAdapter != null) {
                recoveryPhrasesToVerifyAdapter.setRecoveryPhraseMap(
                        recoveryPhrasesAdapter.getSelectedRecoveryPhraseMap());
                recoveryPhrasesToVerifyAdapter.notifyDataSetChanged();
            }

            if (recoveryPhrasesAdapter != null
                    && recoveryPhrasesAdapter.getSelectedRecoveryPhraseMap().size()
                            == recoveryPhrases.size()) {
                recoveryPhraseButton.setAlpha(1f);
                recoveryPhraseButton.setEnabled(true);
            }
        }
    };

    OnRecoveryPhraseSelected onSelectedRecoveryPhraseSelected = new OnRecoveryPhraseSelected() {
        @Override
        public void onSelectedRecoveryPhrase(String phrase) {
            if (recoveryPhrasesAdapter != null) {
                recoveryPhrasesAdapter.addPhraseAtPosition(recoveryPhrases.indexOf(phrase), phrase);
                recoveryPhrasesAdapter.removeSelectedPhrase(phrase);
                recoveryPhrasesAdapter.notifyDataSetChanged();
            }

            if (recoveryPhrasesAdapter != null && recoveryPhrasesToVerifyAdapter != null) {
                recoveryPhrasesToVerifyAdapter.setRecoveryPhraseMap(
                        recoveryPhrasesAdapter.getSelectedRecoveryPhraseMap());
                recoveryPhrasesToVerifyAdapter.notifyDataSetChanged();
            }

            if (recoveryPhrasesAdapter != null
                    && recoveryPhrasesAdapter.getSelectedRecoveryPhraseMap().size()
                            == recoveryPhrases.size()) {
                recoveryPhraseButton.setAlpha(1f);
                recoveryPhraseButton.setEnabled(true);
            }
        }
    };
}
