/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.adapters;

import android.graphics.BlurMaskFilter;
import android.graphics.MaskFilter;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.style.MaskFilterSpan;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.chrome.R;

import java.util.ArrayList;
import java.util.List;

public class RecoveryPhraseAdapter extends RecyclerView.Adapter<RecoveryPhraseAdapter.ViewHolder> {
    private static final String BLANK_SPACES = "   ";
    private static final String MASKED_WORD = "XXXXXXX";
    private List<String> mRecoveryPhraseList = new ArrayList<>();
    private boolean mBlurPhrase;
    private final MaskFilterSpan mMaskFilterSpan;

    public RecoveryPhraseAdapter() {
        mBlurPhrase = true;

        MaskFilter blurMask = new BlurMaskFilter(28f, BlurMaskFilter.Blur.NORMAL);
        mMaskFilterSpan = new MaskFilterSpan(blurMask);
    }

    public boolean isBlurred() {
        return mBlurPhrase;
    }

    public void blurPhrase(final boolean blur) {
        if (mBlurPhrase == blur) {
            return;
        }

        mBlurPhrase = blur;
        notifyItemRangeChanged(0, getItemCount());
    }

    @NonNull
    @Override
    public ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
        LayoutInflater layoutInflater = LayoutInflater.from(parent.getContext());
        View listItem = layoutInflater.inflate(R.layout.recovery_phrase_item_layout, parent, false);
        return new ViewHolder(listItem);
    }

    @Override
    public void onBindViewHolder(@NonNull ViewHolder holder, int position) {
        final String recoveryPhrase =
                String.format(
                        holder.recoveryPhraseText
                                .getContext()
                                .getResources()
                                .getString(R.string.recovery_phrase_item_text),
                        (position + 1),
                        mRecoveryPhraseList.get(position));
        // When blurring a word, add blank spaces at the beginning to improve the overall effect,
        // otherwise the blur will be rendered with an ugly cut at the beginning.
        // SECURITY: Blurring the word XXXXXXX and not the real words.
        // ALWAYS USE A PLACEHOLDER TO PREVENT TEXT EXTRACTION WHEN BLURRED!
        final SpannableString recoveryPhraseSpannable =
                mBlurPhrase
                        ? new SpannableString(BLANK_SPACES + MASKED_WORD)
                        : new SpannableString(recoveryPhrase);
        if (mBlurPhrase) {
            recoveryPhraseSpannable.setSpan(
                    mMaskFilterSpan,
                    BLANK_SPACES.length(),
                    recoveryPhraseSpannable.length(),
                    Spanned.SPAN_INCLUSIVE_EXCLUSIVE);
        } else {
            recoveryPhraseSpannable.removeSpan(mMaskFilterSpan);
        }
        holder.recoveryPhraseText.setText(recoveryPhraseSpannable);
    }

    public void setRecoveryPhraseList(List<String> recoveryPhraseList) {
        mRecoveryPhraseList = recoveryPhraseList;
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
