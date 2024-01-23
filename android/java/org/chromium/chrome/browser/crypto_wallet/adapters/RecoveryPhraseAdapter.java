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
import org.chromium.chrome.browser.crypto_wallet.fragments.onboarding.OnboardingVerifyRecoveryPhraseFragment;

import java.util.ArrayList;
import java.util.List;

public class RecoveryPhraseAdapter extends RecyclerView.Adapter<RecoveryPhraseAdapter.ViewHolder> {
    private List<String> mRecoveryPhraseList = new ArrayList<>();
    private final List<String> mSelectedRecoveryPhraseList = new ArrayList<>();
    private final List<Integer> mSelectedPositions = new ArrayList<>();
    private OnboardingVerifyRecoveryPhraseFragment.OnRecoveryPhraseSelected
            mOnRecoveryPhraseSelected;
    private boolean mSelectedRecoveryPhrase;

    @NonNull
    @Override
    public ViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        LayoutInflater layoutInflater = LayoutInflater.from(parent.getContext());
        View listItem = layoutInflater.inflate(R.layout.recovery_phrase_item_layout, parent, false);
        return new ViewHolder(listItem);
    }

    @Override
    public void onBindViewHolder(@NonNull ViewHolder holder, int position) {
        final String recoveryPhrase = mRecoveryPhraseList.get(position);
        holder.recoveryPhraseText.setText(
                String.format(holder.recoveryPhraseText.getContext().getResources().getString(
                                      R.string.recovery_phrase_item_text),
                        (position + 1), recoveryPhrase));
        if (mOnRecoveryPhraseSelected != null) {
            holder.itemView.setOnClickListener(
                    v -> {
                        if (mSelectedRecoveryPhrase) {
                            mRecoveryPhraseList.remove(recoveryPhrase);
                        } else {
                            mSelectedRecoveryPhraseList.add(recoveryPhrase);
                            mSelectedPositions.add(position);
                        }
                        mOnRecoveryPhraseSelected.onSelectedRecoveryPhrase(recoveryPhrase);
                    });
            if (!mSelectedRecoveryPhrase) {
                if (mSelectedRecoveryPhraseList.contains(recoveryPhrase)
                        && mSelectedPositions.contains(position)) {
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

    public void setSelectedRecoveryPhrase(final boolean selectedRecoveryPhrase) {
        mSelectedRecoveryPhrase = selectedRecoveryPhrase;
    }

    public void addPhraseAtPosition(int position, String phrase) {
        mRecoveryPhraseList.set(position, phrase);
    }

    public void removeSelectedPhrase(String phrase) {
        this.mSelectedRecoveryPhraseList.remove(phrase);
        // We have to iterate as words sometimes can be duplicated in the recovery phrase
        for (int i = 0; i < mRecoveryPhraseList.size(); i++) {
            if (mRecoveryPhraseList.get(i).contains(phrase) && mSelectedPositions.contains(i)) {
                mSelectedPositions.remove((Integer) i);
                break;
            }
        }
    }

    public void setRecoveryPhraseList(List<String> recoveryPhraseList) {
        mRecoveryPhraseList = recoveryPhraseList;
    }

    public void setOnRecoveryPhraseSelectedListener(
            OnboardingVerifyRecoveryPhraseFragment.OnRecoveryPhraseSelected
                    onRecoveryPhraseSelected) {
        this.mOnRecoveryPhraseSelected = onRecoveryPhraseSelected;
    }

    public List<String> getSelectedRecoveryPhraseList() {
        return mSelectedRecoveryPhraseList;
    }

    public List<String> getRecoveryPhraseList() {
        return mRecoveryPhraseList;
    }

    @Override
    public int getItemCount() {
        return mRecoveryPhraseList.size();
    }

    public static class ViewHolder extends RecyclerView.ViewHolder {
        public TextView recoveryPhraseText;

        public ViewHolder(View itemView) {
            super(itemView);
            this.recoveryPhraseText = itemView.findViewById(R.id.recovery_phrase_text);
        }
    }
}
