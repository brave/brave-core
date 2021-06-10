/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.adapters;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.fragments.onboarding_fragments.VerifyRecoveryPhraseFragment;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class RecoveryPhraseAdapter extends RecyclerView.Adapter<RecoveryPhraseAdapter.ViewHolder> {
    Map<Integer, String> recoveryPhraseMap = new HashMap<>();
    Map<Integer, String> selectedRecoveryPhraseMap = new HashMap<>();
    private VerifyRecoveryPhraseFragment.OnRecoveryPhraseSelected onRecoveryPhraseSelected;

    @NonNull
    @Override
    public ViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        LayoutInflater layoutInflater = LayoutInflater.from(parent.getContext());
        View listItem = layoutInflater.inflate(R.layout.recovery_phrase_item_layout, parent, false);
        return new ViewHolder(listItem);
    }

    @Override
    public void onBindViewHolder(@NonNull ViewHolder holder, int position) {
        List<Integer> recoveryPhrasePositions = new ArrayList<>(recoveryPhraseMap.keySet());
        int recoveryPhrasePosition = recoveryPhrasePositions.get(position);
        final String recoveryPhrase = recoveryPhraseMap.get(recoveryPhrasePosition);
        if (recoveryPhrase != null) {
            holder.recoveryPhraseText.setText(
                    String.format(holder.recoveryPhraseText.getContext().getResources().getString(
                                          R.string.recovery_phrase_item_text),
                            (position + 1), recoveryPhrase));
            if (onRecoveryPhraseSelected != null) {
                holder.itemView.setOnClickListener(v -> {
                    selectedRecoveryPhraseMap.put(position, recoveryPhrase);
                    onRecoveryPhraseSelected.onSelectedRecoveryPhrase();
                });
                if (selectedRecoveryPhraseMap.containsKey(position)) {
                    holder.recoveryPhraseText.setEnabled(false);
                    holder.recoveryPhraseText.setAlpha(0.5f);
                    holder.recoveryPhraseText.setText("");
                } else {
                    holder.recoveryPhraseText.setEnabled(true);
                    holder.recoveryPhraseText.setAlpha(1f);
                    holder.recoveryPhraseText.setText(String.format(
                            holder.recoveryPhraseText.getContext().getResources().getString(
                                    R.string.recovery_phrase_item_text),
                            (position + 1), recoveryPhrase));
                }
            }
        }
    }

    public void setRecoveryPhraseMap(Map<Integer, String> recoveryPhraseMap) {
        this.recoveryPhraseMap = recoveryPhraseMap;
    }

    public void setOnRecoveryPhraseSelectedListener(
            VerifyRecoveryPhraseFragment.OnRecoveryPhraseSelected onRecoveryPhraseSelected) {
        this.onRecoveryPhraseSelected = onRecoveryPhraseSelected;
    }

    public Map<Integer, String> getSelectedRecoveryPhraseMap() {
        return selectedRecoveryPhraseMap;
    }

    @Override
    public int getItemCount() {
        return recoveryPhraseMap.size();
    }

    public static class ViewHolder extends RecyclerView.ViewHolder {
        public TextView recoveryPhraseText;

        public ViewHolder(View itemView) {
            super(itemView);
            this.recoveryPhraseText = itemView.findViewById(R.id.recovery_phrase_text);
        }
    }
}
