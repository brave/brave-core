/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.custom_layout;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.RelativeLayout;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.content.res.AppCompatResources;

import com.google.android.material.textfield.TextInputEditText;
import com.google.android.material.textfield.TextInputLayout;

import org.chromium.chrome.R;

/** Custom view that shows the strength of a password depending on its length. */
public class PasswordStrengthMeterView extends RelativeLayout {
    /** Listener used to notify the parent hosting the view. */
    public interface PasswordStrengthMeterListener {
        /** Callback that notifies if the two passwords typed match. */
        void onMatch(final boolean match);
    }

    private boolean mShortPassword = true;
    private TextInputLayout mInputRetypeLayout;
    private TextInputEditText mRetype;
    private TextView mMatch;

    @Nullable private PasswordStrengthMeterListener mListener;

    public PasswordStrengthMeterView(Context context) {
        super(context);
        init(context);
    }

    public PasswordStrengthMeterView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init(context);
    }

    public PasswordStrengthMeterView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init(context);
    }

    public PasswordStrengthMeterView(
            Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
        init(context);
    }

    public void setListener(@Nullable PasswordStrengthMeterListener listener) {
        mListener = listener;
    }

    private void init(@NonNull final Context context) {
        LayoutInflater.from(context).inflate(R.layout.password_strength_meter_layout, this, true);

        TextInputLayout inputLayout = findViewById(R.id.text_input_password_layout);
        mInputRetypeLayout = findViewById(R.id.text_input_retype_layout);
        TextInputEditText input = findViewById(R.id.text_input_password_edit_text);
        mRetype = findViewById(R.id.text_input_retype_edit_text);
        TextView weak = findViewById(R.id.password_strength_weak);
        TextView medium = findViewById(R.id.password_strength_medium);
        TextView strong = findViewById(R.id.password_strength_strong);
        mMatch = findViewById(R.id.password_match);

        weak.measure(MeasureSpec.UNSPECIFIED, MeasureSpec.UNSPECIFIED);
        int weakWidth = weak.getMeasuredWidth() + weak.getCompoundDrawablePadding();

        medium.measure(MeasureSpec.UNSPECIFIED, MeasureSpec.UNSPECIFIED);
        int mediumWidth = medium.getMeasuredWidth() + medium.getCompoundDrawablePadding();

        strong.measure(MeasureSpec.UNSPECIFIED, MeasureSpec.UNSPECIFIED);
        int strongWidth = strong.getMeasuredWidth() + strong.getCompoundDrawablePadding();

        mMatch.measure(MeasureSpec.UNSPECIFIED, MeasureSpec.UNSPECIFIED);
        int matchWidth = mMatch.getMeasuredWidth() + mMatch.getCompoundDrawablePadding();

        int maxWidth =
                Math.max(Math.max(weakWidth, Math.max(mediumWidth, strongWidth)), matchWidth);

        Drawable progressBar =
                AppCompatResources.getDrawable(context, R.drawable.progress_bar_strong);
        final int progressBarWidth = progressBar != null ? progressBar.getMinimumWidth() : 0;

        // Calculate right padding.
        input.setPadding(
                input.getPaddingLeft(),
                input.getPaddingTop(),
                maxWidth + progressBarWidth,
                input.getPaddingBottom());
        mRetype.setPadding(
                mRetype.getPaddingLeft(),
                mRetype.getPaddingTop(),
                maxWidth + progressBarWidth,
                mRetype.getPaddingBottom());

        // Calculate left margin for text views.
        input.post(
                () -> {
                    int width = input.getWidth() - input.getPaddingEnd();
                    setLeftMargin(weak, width);
                    setLeftMargin(medium, width);
                    setLeftMargin(strong, width);
                    setLeftMargin(mMatch, width);
                });

        input.addTextChangedListener(
                new TextWatcher() {
                    @Override
                    public void beforeTextChanged(
                            CharSequence charSequence, int start, int count, int after) {
                        // Unused.
                    }

                    @Override
                    public void onTextChanged(
                            CharSequence charSequence, int start, int before, int count) {
                        final int length = charSequence.length();
                        if (length < 8) {
                            notifyListener(false);
                            mMatch.setVisibility(View.INVISIBLE);
                            mRetype.setCompoundDrawablesRelativeWithIntrinsicBounds(0, 0, 0, 0);
                            mInputRetypeLayout.setError(null);
                            mShortPassword = true;
                        } else {
                            mShortPassword = false;
                            checkPasswords(charSequence, mRetype);
                        }

                        if (length == 0 || length >= 8) {
                            input.setCompoundDrawablesRelativeWithIntrinsicBounds(0, 0, 0, 0);
                            inputLayout.setError(null);
                        } else {
                            input.setCompoundDrawablesRelativeWithIntrinsicBounds(
                                    0, 0, R.drawable.warning_circle_filled, 0);
                            inputLayout.setError(
                                    getContext()
                                            .getString(R.string.wallet_password_minimum_length));
                        }

                        if (length < 12) {
                            weak.setVisibility(View.VISIBLE);
                            medium.setVisibility(View.INVISIBLE);
                            strong.setVisibility(View.INVISIBLE);
                        } else if (length < 16) {
                            weak.setVisibility(View.INVISIBLE);
                            medium.setVisibility(View.VISIBLE);
                            strong.setVisibility(View.INVISIBLE);
                        } else {
                            weak.setVisibility(View.INVISIBLE);
                            medium.setVisibility(View.INVISIBLE);
                            strong.setVisibility(View.VISIBLE);
                        }
                    }

                    @Override
                    public void afterTextChanged(Editable editable) {
                        // Unused.
                    }
                });

        mRetype.addTextChangedListener(
                new TextWatcher() {
                    @Override
                    public void beforeTextChanged(
                            CharSequence charSequence, int start, int count, int after) {
                        // Unused.
                    }

                    @Override
                    public void onTextChanged(
                            CharSequence charSequence, int start, int before, int count) {
                        // Do not check for match until password length error is fixed.
                        if (!mShortPassword) {
                            checkPasswords(charSequence, input);
                        }
                    }

                    @Override
                    public void afterTextChanged(Editable editable) {
                        // Unused.
                    }
                });
    }

    private void notifyListener(final boolean match) {
        if (mListener != null) {
            mListener.onMatch(match);
        }
    }

    private void checkPasswords(
            @NonNull final CharSequence charSequence,
            @NonNull final TextInputEditText inputEditText) {
        if (passwordsMatch(charSequence, inputEditText)) {
            notifyListener(true);
            mMatch.setVisibility(View.VISIBLE);
            mRetype.setCompoundDrawablesRelativeWithIntrinsicBounds(0, 0, 0, 0);
            mInputRetypeLayout.setError(null);
        } else if (mRetype.getText() == null || mRetype.getText().length() == 0) {
            notifyListener(false);
            mMatch.setVisibility(View.INVISIBLE);
            mRetype.setCompoundDrawablesRelativeWithIntrinsicBounds(0, 0, 0, 0);
            mInputRetypeLayout.setError(null);
        } else {
            notifyListener(false);
            mMatch.setVisibility(View.INVISIBLE);
            mRetype.setCompoundDrawablesRelativeWithIntrinsicBounds(
                    0, 0, R.drawable.warning_circle_filled, 0);
            mInputRetypeLayout.setError(
                    getContext().getString(R.string.wallet_password_does_not_match));
        }
    }

    private boolean passwordsMatch(
            @NonNull final CharSequence charSequence,
            @NonNull final TextInputEditText inputEditText) {
        if (inputEditText.getText() == null) {
            return false;
        }
        return charSequence.toString().equals(inputEditText.getText().toString());
    }

    private void setLeftMargin(@NonNull final TextView textView, final int width) {
        ViewGroup.LayoutParams layoutParams = textView.getLayoutParams();
        if (layoutParams instanceof RelativeLayout.LayoutParams relativeLayoutParams) {
            relativeLayoutParams.leftMargin = width;
            textView.setLayoutParams(relativeLayoutParams);
        }
    }
}
