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

import java.util.ArrayList;
import java.util.List;

import org.chromium.chrome.R;

public class RecoveryPhraseAdapter extends RecyclerView.Adapter<RecoveryPhraseAdapter.RecoveryPhraseViewHolder> {

    List<String> recoveryPhraseList = new ArrayList<>();

    @NonNull
    @Override
    public RecoveryPhraseViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        LayoutInflater layoutInflater = LayoutInflater.from(parent.getContext());
        View listItem = layoutInflater.inflate(R.layout.recovery_phrase_item_layout, parent, false);
        return new RecoveryPhraseViewHolder(listItem);
    }

    @Override
    public void onBindViewHolder(RecoveryPhraseViewHolder holder, int position) {
        final String recoveryPhrase = recoveryPhraseList.get(position);
        holder.recoveryPhraseText.setText(String.format(holder.recoveryPhraseText.getContext().getResources().getString(R.string.recovery_phrase_item_text), (position + 1), recoveryPhrase));
    }

    public void setRecoveryPhraseList(List<String> recoveryPhraseList) {
        this.recoveryPhraseList = recoveryPhraseList;
    }

    @Override
    public int getItemCount() {
        return recoveryPhraseList.size();
    }

    public static class RecoveryPhraseViewHolder extends RecyclerView.ViewHolder {
        public TextView recoveryPhraseText;

        public RecoveryPhraseViewHolder(View itemView) {
            super(itemView);
            this.recoveryPhraseText = itemView.findViewById(R.id.recovery_phrase_text);
        }
    }
}
