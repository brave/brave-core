/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments.onboarding;

import static org.chromium.chrome.browser.crypto_wallet.util.Utils.hideKeyboard;

import android.content.ClipboardManager;
import android.content.Context;
import android.os.Bundle;
import android.text.Editable;
import android.text.InputType;
import android.text.TextWatcher;
import android.text.method.PasswordTransformationMethod;
import android.view.ContextThemeWrapper;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.CheckBox;
import android.widget.GridLayout;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.widget.AppCompatButton;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.custom_layout.PasteEditText;
import org.chromium.ui.base.ViewUtils;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

public class OnboardingRestoreWalletFragment extends BaseOnboardingWalletFragment
        implements PasteEditText.Listener, View.OnFocusChangeListener {

    private static final int RECOVERY_PHRASE_12_WORDS = 12;
    private static final int RECOVERY_PHRASE_24_WORDS = 24;

    /** Grid layout containing 12-word phrase. */
    private GridLayout mGridLayout12;

    /** Grid layout containing the second part of words for a 24-word phrase */
    private GridLayout mGridLayout24;

    private TextView mSwitchRecoveryPhrase;
    private AppCompatButton mButtonContinue;
    private ImageView mToggleWordMask;
    private CheckBox mLegacyImport;

    @Nullable private ClipboardManager mClipboard;

    /** List of string containing split words pasted from clipboard. */
    private List<String> mPastedWords;

    private List<PasteEditText> mGridLayout12List;
    private List<PasteEditText> mGridLayout24List;

    /** Flag indicating whether the words are visible or masked. */
    private boolean mMaskWord;

    /** Current word count. It can be 12 or 24. */
    private int mWordCount;

    /**
     * Set of filled items used to notify the listener that enables or disables the button
     * visibility.
     */
    private Set<Integer> mFilledItems;

    /**
     * View that keeps track of the last focused items, used to hide the keyboard in case the focus
     * belongs to an item that is being hidden.
     */
    private View mLastFocusedItem;

    private boolean mLegacyWalletRestoreEnabled;

    @NonNull
    public static OnboardingRestoreWalletFragment newInstance() {
        return new OnboardingRestoreWalletFragment();
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // As initial state 12-words phrase is shown.
        mWordCount = RECOVERY_PHRASE_12_WORDS;
        mGridLayout12List = new ArrayList<>(12);
        mGridLayout24List = new ArrayList<>(12);
        mPastedWords = new ArrayList<>();
        // As initial state words are masked.
        mMaskWord = true;
        mFilledItems = new HashSet<>();
        mClipboard = requireContext().getSystemService(ClipboardManager.class);
    }

    @Override
    public View onCreateView(
            @NonNull LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_restore_wallet, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        mGridLayout12 = view.findViewById(R.id.recovery_phrase_12);
        // Populate the first 12 items in the grid layout.
        for (int i = 0; i < RECOVERY_PHRASE_12_WORDS; i++) {
            PasteEditText pasteEditText = generatePasteEditText(i == mWordCount - 1, i);
            mGridLayout12List.add(pasteEditText);
            mGridLayout12.addView(pasteEditText, generateGridLayoutParams());
        }

        mGridLayout24 = view.findViewById(R.id.recovery_phrase_24);
        // Populate the second grid layout with the second batch of 12 items.
        for (int i = RECOVERY_PHRASE_12_WORDS; i < RECOVERY_PHRASE_24_WORDS; i++) {
            PasteEditText pasteEditText =
                    generatePasteEditText(i == RECOVERY_PHRASE_24_WORDS - 1, i);
            mGridLayout24List.add(pasteEditText);
            mGridLayout24.addView(pasteEditText, generateGridLayoutParams());
        }

        mLegacyImport = view.findViewById(R.id.legacy_import);
        mLegacyImport.setOnCheckedChangeListener(
                (buttonView, isChecked) -> mLegacyWalletRestoreEnabled = isChecked);

        if (mWordCount != RECOVERY_PHRASE_24_WORDS) {
            mGridLayout24.setVisibility(View.GONE);
        }

        mButtonContinue = view.findViewById(R.id.continue_button);
        mButtonContinue.setOnClickListener(
                (View.OnClickListener)
                        v -> {
                            String recoverPhrase = extractRecoveryPhrase();
                            goToNextPage(recoverPhrase);
                        });

        mSwitchRecoveryPhrase = view.findViewById(R.id.recovery_phrase_switch);
        mSwitchRecoveryPhrase.setOnClickListener(
                v -> {
                    mWordCount =
                            mWordCount == RECOVERY_PHRASE_12_WORDS
                                    ? RECOVERY_PHRASE_24_WORDS
                                    : RECOVERY_PHRASE_12_WORDS;
                    mGridLayout24.setVisibility(
                            mWordCount == RECOVERY_PHRASE_24_WORDS ? View.VISIBLE : View.GONE);
                    if (mWordCount == RECOVERY_PHRASE_12_WORDS) {
                        mLegacyImport.setChecked(false);
                        int i = RECOVERY_PHRASE_12_WORDS;
                        for (PasteEditText pasteEditText : mGridLayout24List) {

                            if (mLastFocusedItem != null
                                    && pasteEditText.getId() == mLastFocusedItem.getId()) {
                                hideKeyboard(requireActivity(), pasteEditText.getWindowToken());
                            }

                            pasteEditText.setText("");
                            mFilledItems.remove(i);
                            i++;
                        }
                    }
                    mButtonContinue.setEnabled(mFilledItems.size() == mWordCount);
                    mLegacyImport.setVisibility(
                            mWordCount == RECOVERY_PHRASE_12_WORDS ? View.INVISIBLE : View.VISIBLE);

                    final PasteEditText pasteEditText =
                            mGridLayout12List.get(RECOVERY_PHRASE_12_WORDS - 1);
                    pasteEditText.setImeOptions(
                            mWordCount == RECOVERY_PHRASE_12_WORDS
                                    ? EditorInfo.IME_ACTION_DONE
                                    : EditorInfo.IME_ACTION_NEXT);
                    if (pasteEditText.hasFocus()) {
                        InputMethodManager imm =
                                (InputMethodManager)
                                        requireContext()
                                                .getSystemService(Context.INPUT_METHOD_SERVICE);
                        imm.restartInput(pasteEditText);
                    }
                    mSwitchRecoveryPhrase.setText(
                            mWordCount == RECOVERY_PHRASE_12_WORDS
                                    ? R.string.recovery_phrase_24
                                    : R.string.recovery_phrase_12);
                });

        mToggleWordMask = view.findViewById(R.id.toggle_word_mask);
        mToggleWordMask.setOnClickListener(
                v -> {
                    toggleWordMask();
                    mToggleWordMask.setImageResource(
                            mMaskWord ? R.drawable.ic_eye_on : R.drawable.ic_eye_off);
                });
    }

    @NonNull
    private String getClipboardText() {
        if (mClipboard == null || mClipboard.getPrimaryClip() == null) {
            return "";
        }

        return mClipboard.getPrimaryClip().getItemAt(0).getText().toString();
    }

    private void splitWords(@NonNull final String pastedText) {
        List<String> words = Arrays.asList(pastedText.split("\\s+"));
        if (words.size() > mWordCount) {
            words = words.subList(0, mWordCount);
        }
        mPastedWords.clear();
        mPastedWords.addAll(words);
    }

    private void fillAvailableItems() {
        List<PasteEditText> availableItems = getVisibleEditTextList();
        final int size = Math.min(availableItems.size(), mPastedWords.size());
        for (int i = 0; i < size; i++) {
            PasteEditText pasteEditText = availableItems.get(i);
            pasteEditText.setText(mPastedWords.get(i));
        }
    }

    @NonNull
    private List<PasteEditText> getVisibleEditTextList() {
        final List<PasteEditText> result = new ArrayList<>(mWordCount);
        result.addAll(mGridLayout12List);
        if (mWordCount == RECOVERY_PHRASE_24_WORDS) {
            result.addAll(mGridLayout24List);
        }
        return result;
    }

    @NonNull
    private PasteEditText generatePasteEditText(final boolean lastItem, final int position) {
        // Apply context wrapper to introduce Brave Wallet colors and styles.
        ContextThemeWrapper contextThemeWrapper =
                new ContextThemeWrapper(requireContext(), R.style.BraveWalletEditTextTheme);
        PasteEditText pasteEditText = new PasteEditText(contextThemeWrapper);
        pasteEditText.setListener(this);
        pasteEditText.setInputType(InputType.TYPE_CLASS_TEXT);
        pasteEditText.setAutofillHints("recoveryPhrase");
        // Set correct IME option to the last item, so the keyboard will
        // jump to the next item or hide accordingly.
        pasteEditText.setImeOptions(
                lastItem ? EditorInfo.IME_ACTION_DONE : EditorInfo.IME_ACTION_NEXT);
        pasteEditText.setHint(
                String.format(
                        getResources().getString(R.string.recovery_phrase_word_hint),
                        position + 1));
        pasteEditText.setTransformationMethod(
                mMaskWord ? new PasswordTransformationMethod() : null);
        pasteEditText.addTextChangedListener(
                new TextWatcher() {
                    @Override
                    public void beforeTextChanged(CharSequence s, int start, int count, int after) {
                        /* Not used. */
                    }

                    @Override
                    public void onTextChanged(CharSequence s, int start, int before, int count) {
                        if (s.toString().trim().length() == 0) {
                            mFilledItems.remove(position);
                        } else {
                            mFilledItems.add(position);
                        }

                        // Enable or disable continue button when all items are filled.
                        mButtonContinue.setEnabled(mFilledItems.size() == mWordCount);
                    }

                    @Override
                    public void afterTextChanged(Editable s) {
                        /* Not used. */
                    }
                });
        pasteEditText.setOnFocusChangeListener(this);
        pasteEditText.setId(View.generateViewId());
        return pasteEditText;
    }

    @NonNull
    private GridLayout.LayoutParams generateGridLayoutParams() {
        GridLayout.LayoutParams gridLayoutParams =
                new GridLayout.LayoutParams(
                        new ViewGroup.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT));
        gridLayoutParams.columnSpec = GridLayout.spec(GridLayout.UNDEFINED, 1f);
        gridLayoutParams.bottomMargin = ViewUtils.dpToPx(requireContext(), 8);
        gridLayoutParams.leftMargin = ViewUtils.dpToPx(requireContext(), 4);
        gridLayoutParams.rightMargin = ViewUtils.dpToPx(requireContext(), 4);
        return gridLayoutParams;
    }

    @NonNull
    private String extractRecoveryPhrase() {
        final StringBuilder result = new StringBuilder();
        final List<PasteEditText> visibleEditText = getVisibleEditTextList();
        final int size = visibleEditText.size();
        for (int i = 0; i < size; i++) {
            Editable editable = visibleEditText.get(i).getText();
            if (editable != null && !editable.toString().trim().isEmpty()) {
                result.append(editable.toString().trim());
                if (i != size - 1) {
                    result.append(" ");
                }
            }
        }
        return result.toString();
    }

    private void goToNextPage(@NonNull final String recoveryPhrase) {
        mOnboardingViewModel.setRecoveryPhrase(recoveryPhrase);
        mOnboardingViewModel.setLegacyRestoreEnabled(mLegacyWalletRestoreEnabled);

        Utils.clearClipboard(recoveryPhrase);
        mPastedWords.clear();
        mGridLayout12List.clear();
        mGridLayout24List.clear();
        mFilledItems.clear();

        if (mOnNextPage != null) {
            mOnNextPage.incrementPages(1);
        }
    }

    private void toggleWordMask() {
        mMaskWord = !mMaskWord;
        for (PasteEditText pasteEditText : mGridLayout12List) {
            pasteEditText.setTransformationMethod(
                    mMaskWord ? new PasswordTransformationMethod() : null);
        }

        for (PasteEditText pasteEditText : mGridLayout24List) {
            pasteEditText.setTransformationMethod(
                    mMaskWord ? new PasswordTransformationMethod() : null);
        }
    }

    @Override
    public void onPaste() {
        final String pastedText = getClipboardText();
        splitWords(pastedText);
        fillAvailableItems();
    }

    @Override
    public void onFocusChange(View view, boolean hasFocus) {
        if (hasFocus) {
            mLastFocusedItem = view;
        }
    }
}
