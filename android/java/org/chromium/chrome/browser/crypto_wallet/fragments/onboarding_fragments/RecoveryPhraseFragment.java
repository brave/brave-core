/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments.onboarding_fragments;

import android.app.Activity;
import android.os.Bundle;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.FragmentActivity;
import androidx.recyclerview.widget.GridLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletActivity;
import org.chromium.chrome.browser.crypto_wallet.adapters.RecoveryPhraseAdapter;
import org.chromium.chrome.browser.crypto_wallet.util.ItemOffsetDecoration;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.init.AsyncInitializationActivity;
import org.chromium.ui.base.WindowAndroid;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class RecoveryPhraseFragment extends CryptoOnboardingFragment {
    private List<String> recoveryPhrases;

    private KeyringService getKeyringService() {
        Activity activity = getActivity();
        if (activity instanceof BraveWalletActivity) {
            return ((BraveWalletActivity) activity).getKeyringService();
        }

        return null;
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_recovery_phrase, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        KeyringService keyringService = getKeyringService();
        if (keyringService != null) {
            keyringService.getMnemonicForDefaultKeyring(result -> {
                recoveryPhrases = Utils.getRecoveryPhraseAsList(result);
                setupRecoveryPhraseRecyclerView(view);
                TextView copyButton = view.findViewById(R.id.btn_copy);
                assert getActivity() != null;
                copyButton.setOnClickListener(v -> {
                    Utils.saveTextToClipboard(getActivity(),
                            Utils.getRecoveryPhraseFromList(recoveryPhrases),
                            R.string.text_has_been_copied, true);
                });
                setupRecoveryPhraseRecyclerView(view);
            });
        }

        Button recoveryPhraseButton = view.findViewById(R.id.btn_recovery_phrase_continue);
        recoveryPhraseButton.setOnClickListener(v -> onNextPage.gotoNextPage(false));
        CheckBox recoveryPhraseCheckbox = view.findViewById(R.id.recovery_phrase_checkbox);
        recoveryPhraseCheckbox.setOnCheckedChangeListener((buttonView, isChecked) -> {
            if (isChecked) {
                recoveryPhraseButton.setEnabled(true);
                recoveryPhraseButton.setAlpha(1.0f);
            } else {
                recoveryPhraseButton.setEnabled(false);
                recoveryPhraseButton.setAlpha(0.5f);
            }
        });
        TextView recoveryPhraseSkipButton = view.findViewById(R.id.btn_recovery_phrase_skip);
        recoveryPhraseSkipButton.setOnClickListener(v -> onNextPage.gotoNextPage(true));
    }

    private void setupRecoveryPhraseRecyclerView(View view) {
        RecyclerView recyclerView = view.findViewById(R.id.recovery_phrase_recyclerview);
        assert getActivity() != null;
        recyclerView.addItemDecoration(
                new ItemOffsetDecoration(getActivity(), R.dimen.zero_margin));
        GridLayoutManager layoutManager = new GridLayoutManager(getActivity(), 3);
        recyclerView.setLayoutManager(layoutManager);

        RecoveryPhraseAdapter recoveryPhraseAdapter = new RecoveryPhraseAdapter();
        recoveryPhraseAdapter.setRecoveryPhraseList(recoveryPhrases);
        recyclerView.setAdapter(recoveryPhraseAdapter);
    }
}
