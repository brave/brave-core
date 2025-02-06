/**
 * Copyright (c) 2025 The Brave Authors. All rights reserved. This Source Code Form is subject to
 * the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with
 * this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
package org.chromium.brave.browser.search_engines.settings;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.google.android.material.textfield.TextInputEditText;
import com.google.android.material.textfield.TextInputLayout;

import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.ObservableSupplierImpl;
import org.chromium.chrome.browser.search_engines.R;
import org.chromium.chrome.browser.settings.ChromeBaseSettingsFragment;

public class AddCustomSearchEnginePreferenceFragment extends ChromeBaseSettingsFragment {
    private TextInputEditText mTitleEdittext;
    private TextInputLayout mTitleLayout;

    private TextInputEditText mUrlEdittext;
    private TextInputLayout mUrlLayout;

    private final ObservableSupplierImpl<String> mPageTitle = new ObservableSupplierImpl<>();

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mPageTitle.set(getString(R.string.add_custom_search_engine));
    }

    @Nullable
    @Override
    public View onCreateView(
            @NonNull LayoutInflater inflater,
            @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        // Inflate custom LinearLayout
        View rootView =
                inflater.inflate(
                        R.layout.add_custom_search_engine_preference_layout, container, false);

        mTitleEdittext = (TextInputEditText) rootView.findViewById(R.id.title_edittext);
        mTitleLayout = (TextInputLayout) rootView.findViewById(R.id.title_layout);

        mUrlEdittext = (TextInputEditText) rootView.findViewById(R.id.url_edittext);
        mUrlLayout = (TextInputLayout) rootView.findViewById(R.id.url_layout);

        TextView addCustomSeText = (TextView) rootView.findViewById(R.id.add_custom_se_text);
        String addCustomSeString =
                getResources().getString(R.string.add_custom_search_engine_text1)
                        + "\n\n"
                        + getResources().getString(R.string.add_custom_search_engine_text2);
        addCustomSeText.setText(addCustomSeString);

        Button cancelButton = (Button) rootView.findViewById(R.id.cancel_button);
        cancelButton.setOnClickListener(
                new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        getActivity().finish();
                    }
                });

        Button addSearchEngineButton =
                (Button) rootView.findViewById(R.id.add_search_engine_button);
        addSearchEngineButton.setOnClickListener(
                new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        // addSearchEngine();
                    }
                });

        return rootView;
    }

    @Override
    public void onCreatePreferences(Bundle bundle, String s) {
        setHasOptionsMenu(true);
    }

    @Override
    public ObservableSupplier<String> getPageTitle() {
        return mPageTitle;
    }
}
