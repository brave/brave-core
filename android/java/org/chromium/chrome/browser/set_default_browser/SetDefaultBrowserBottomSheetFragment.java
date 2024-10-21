/**
 * Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.set_default_browser;

import android.content.DialogInterface;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.CheckBox;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.google.android.material.bottomsheet.BottomSheetBehavior;
import com.google.android.material.bottomsheet.BottomSheetDialog;
import com.google.android.material.bottomsheet.BottomSheetDialogFragment;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;

public class SetDefaultBrowserBottomSheetFragment extends BottomSheetDialogFragment {
    private static final String IS_FROM_MENU = "is_from_menu";

    private boolean isFromMenu;

    public static SetDefaultBrowserBottomSheetFragment newInstance(boolean isFromMenu) {
        final SetDefaultBrowserBottomSheetFragment fragment =
                new SetDefaultBrowserBottomSheetFragment();
        final Bundle args = new Bundle();
        args.putBoolean(IS_FROM_MENU, isFromMenu);
        fragment.setArguments(args);
        return fragment;
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setStyle(STYLE_NORMAL, R.style.AppSetDefaultBottomSheetDialogTheme);

        if (getArguments() != null) {
            isFromMenu = getArguments().getBoolean(IS_FROM_MENU);
        }
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(
                R.layout.fragment_set_default_browser_bottom_sheet, container, false);
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        ((BottomSheetDialog) getDialog())
                .getBehavior()
                .setState(BottomSheetBehavior.STATE_EXPANDED);
        Button nextButton = view.findViewById(R.id.btn_next);
        nextButton.setOnClickListener(
                new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        if (getActivity() != null) {
                            BraveSetDefaultBrowserUtils.setDefaultBrowser(getActivity());
                        }
                        dismiss();
                    }
                });

        CheckBox dontAskCheckBox = view.findViewById(R.id.checkbox_dont_ask);

        int braveDefaultModalCount =
                ChromeSharedPreferences.getInstance()
                        .readInt(BravePreferenceKeys.BRAVE_SET_DEFAULT_BOTTOM_SHEET_COUNT);

        if (braveDefaultModalCount > 2 && !isFromMenu) {
            dontAskCheckBox.setVisibility(View.VISIBLE);
        } else {
            dontAskCheckBox.setVisibility(View.GONE);
        }

        dontAskCheckBox.setOnCheckedChangeListener((buttonView, isChecked) -> {
            if (isChecked) {
                nextButton.setEnabled(false);
                nextButton.setBackgroundResource(R.drawable.set_default_rounded_button_disabled);
            } else {
                nextButton.setEnabled(true);
                nextButton.setBackgroundResource(R.drawable.orange_rounded_button);
            }
        });

        Button cancelButton = view.findViewById(R.id.btn_cancel);
        cancelButton.setOnClickListener(
                new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        if (dontAskCheckBox.isChecked()) {
                            BraveSetDefaultBrowserUtils.setBraveDefaultDontAsk();
                        }
                        dismiss();
                    }
                });
    }

    @Override
    public void onDismiss(@NonNull DialogInterface dialog) {
        super.onDismiss(dialog);
        BraveSetDefaultBrowserUtils.isBottomSheetVisible = false;
    }
}
