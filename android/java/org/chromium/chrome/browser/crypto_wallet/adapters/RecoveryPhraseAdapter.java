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
import org.chromium.base.Log;
import org.chromium.chrome.browser.crypto_wallet.fragments.onboarding_fragments.VerifyRecoveryPhraseFragment;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class RecoveryPhraseAdapter extends RecyclerView.Adapter<RecoveryPhraseAdapter.ViewHolder> {
    List<String> recoveryPhraseMap = new ArrayList<>();
    List<String> selectedRecoveryPhraseMap = new ArrayList<>();
    private VerifyRecoveryPhraseFragment.OnRecoveryPhraseSelected onRecoveryPhraseSelected;
    private boolean isSelectedRecoveryPhrase;

    @NonNull
    @Override
    public ViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        LayoutInflater layoutInflater = LayoutInflater.from(parent.getContext());
        View listItem = layoutInflater.inflate(R.layout.recovery_phrase_item_layout, parent, false);
        return new ViewHolder(listItem);
    }

    @Override
    public void onBindViewHolder(@NonNull ViewHolder holder, int position) {
        final String recoveryPhrase = recoveryPhraseMap.get(position);
        holder.recoveryPhraseText.setText(String.format(
                            holder.recoveryPhraseText.getContext().getResources().getString(
                                    R.string.recovery_phrase_item_text),
                            (position + 1), recoveryPhrase));
        if (onRecoveryPhraseSelected != null) {
            holder.itemView.setOnClickListener(v -> {
                if (isSelectedRecoveryPhrase) {
                    recoveryPhraseMap.remove(recoveryPhrase);
                } else {
                    selectedRecoveryPhraseMap.add(recoveryPhrase);
                }
                    onRecoveryPhraseSelected.onSelectedRecoveryPhrase(recoveryPhrase);
                });
            if (!isSelectedRecoveryPhrase) {
                if (selectedRecoveryPhraseMap.contains(recoveryPhrase)) {
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

    public void setSelectedRecoveryPhrase(boolean isSelectedRecoveryPhrase) {
        this.isSelectedRecoveryPhrase = isSelectedRecoveryPhrase;
    }

    public void addPhraseAtPosition(int position, String phrase) {
        this.recoveryPhraseMap.set(position, phrase);
    }

    public void removeSelectedPhrase(String phrase) {
        this.selectedRecoveryPhraseMap.remove(phrase);
    }

    public void setRecoveryPhraseMap(List<String> recoveryPhraseMap) {
        this.recoveryPhraseMap = recoveryPhraseMap;
    }

    public void setOnRecoveryPhraseSelectedListener(
            VerifyRecoveryPhraseFragment.OnRecoveryPhraseSelected onRecoveryPhraseSelected) {
        this.onRecoveryPhraseSelected = onRecoveryPhraseSelected;
    }

    public List<String> getSelectedRecoveryPhraseMap() {
        return selectedRecoveryPhraseMap;
    }

    public List<String> getRecoveryPhraseMap() {
        return recoveryPhraseMap;
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
