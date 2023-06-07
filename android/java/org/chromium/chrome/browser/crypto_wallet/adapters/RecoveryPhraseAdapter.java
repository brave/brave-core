/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

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
import java.util.List;

public class RecoveryPhraseAdapter extends RecyclerView.Adapter<RecoveryPhraseAdapter.ViewHolder> {
    List<String> recoveryPhraseList = new ArrayList<>();
    List<String> selectedRecoveryPhraseList = new ArrayList<>();
    List<Integer> selectedPositions = new ArrayList<>();
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
        final String recoveryPhrase = recoveryPhraseList.get(position);
        holder.recoveryPhraseText.setText(
                String.format(holder.recoveryPhraseText.getContext().getResources().getString(
                                      R.string.recovery_phrase_item_text),
                        (position + 1), recoveryPhrase));
        if (onRecoveryPhraseSelected != null) {
            holder.itemView.setOnClickListener(v -> {
                if (isSelectedRecoveryPhrase) {
                    recoveryPhraseList.remove(recoveryPhrase);
                } else {
                    selectedRecoveryPhraseList.add(recoveryPhrase);
                    selectedPositions.add(position);
                }
                onRecoveryPhraseSelected.onSelectedRecoveryPhrase(recoveryPhrase);
            });
            if (!isSelectedRecoveryPhrase) {
                if (selectedRecoveryPhraseList.contains(recoveryPhrase)
                        && selectedPositions.contains(position)) {
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
        this.recoveryPhraseList.set(position, phrase);
    }

    public void removeSelectedPhrase(String phrase) {
        this.selectedRecoveryPhraseList.remove(phrase);
        // We have to iterate as words sometimes can be duplicated in the recovery phrase
        for (int i = 0; i < recoveryPhraseList.size(); i++) {
            if (recoveryPhraseList.get(i).contains(phrase) && selectedPositions.contains(i)) {
                selectedPositions.remove((Integer) i);
                break;
            }
        }
    }

    public void setRecoveryPhraseList(List<String> recoveryPhraseList) {
        this.recoveryPhraseList = recoveryPhraseList;
    }

    public void setOnRecoveryPhraseSelectedListener(
            VerifyRecoveryPhraseFragment.OnRecoveryPhraseSelected onRecoveryPhraseSelected) {
        this.onRecoveryPhraseSelected = onRecoveryPhraseSelected;
    }

    public List<String> getSelectedRecoveryPhraseList() {
        return selectedRecoveryPhraseList;
    }

    public List<String> getRecoveryPhraseList() {
        return recoveryPhraseList;
    }

    @Override
    public int getItemCount() {
        return recoveryPhraseList.size();
    }

    public static class ViewHolder extends RecyclerView.ViewHolder {
        public TextView recoveryPhraseText;

        public ViewHolder(View itemView) {
            super(itemView);
            this.recoveryPhraseText = itemView.findViewById(R.id.recovery_phrase_text);
        }
    }
}
