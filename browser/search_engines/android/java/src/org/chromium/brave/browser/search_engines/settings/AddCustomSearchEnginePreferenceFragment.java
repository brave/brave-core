/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.search_engines.settings;

import android.app.Activity;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

import androidx.activity.OnBackPressedCallback;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.widget.Toolbar;

import com.google.android.material.textfield.TextInputEditText;
import com.google.android.material.textfield.TextInputLayout;

import org.chromium.base.Log;
import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.ObservableSupplierImpl;
import org.chromium.chrome.browser.search_engines.R;
import org.chromium.chrome.browser.search_engines.TemplateUrlServiceFactory;
import org.chromium.chrome.browser.settings.ChromeBaseSettingsFragment;
import org.chromium.components.search_engines.BraveTemplateUrlService;
import org.chromium.components.search_engines.TemplateUrl;
import org.chromium.components.search_engines.TemplateUrlService;
import org.chromium.ui.widget.Toast;

import java.util.List;
import java.util.Locale;

public class AddCustomSearchEnginePreferenceFragment extends ChromeBaseSettingsFragment {
    private TextInputEditText mTitleEdittext;
    private TextInputLayout mTitleLayout;

    private TextInputEditText mUrlEdittext;
    private TextInputLayout mUrlLayout;

    private boolean mIsEditMode;

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

        Button cancelButton = (Button) rootView.findViewById(R.id.cancel_button);
        Button addSearchEngineButton =
                (Button) rootView.findViewById(R.id.add_search_engine_button);

        String searchEngineKeyword = getArguments().getString("keyword");
        Log.e("brave_search", "AddCustomSearchEnginePreferenceFragment 1 : " + searchEngineKeyword);
        if (searchEngineKeyword != null) {
            Runnable templateUrlServiceReady =
                    () -> {
                        TemplateUrl templateUrl = getTemplateUrlByKeyword(searchEngineKeyword);
                        if (templateUrl != null) {
                            Log.e(
                                    "brave_search",
                                    "AddCustomSearchEnginePreferenceFragment 2 : "
                                            + templateUrl.getShortName());
                            mTitleEdittext.setText(templateUrl.getShortName());
                            mUrlEdittext.setText(templateUrl.getURL());
                            addSearchEngineButton.setText(
                                    getString(R.string.save_changes_action_text));
                            mIsEditMode = true;
                        }
                    };
            TemplateUrlServiceFactory.getForProfile(getProfile())
                    .runWhenLoaded(templateUrlServiceReady);

            Activity activity = getActivity();
            if (activity != null) {
                Toolbar actionBar = activity.findViewById(R.id.action_bar);
                if (actionBar != null) {
                    actionBar.setTitle(getString(R.string.edit_custom_search_engine));
                }
            }
            // mPageTitle.set(getString(R.string.edit_custom_search_engine));
        }

        TextView addCustomSeText = (TextView) rootView.findViewById(R.id.add_custom_se_text);
        String addCustomSeString =
                getResources().getString(R.string.add_custom_search_engine_text1)
                        + "\n\n"
                        + getResources().getString(R.string.add_custom_search_engine_text2);
        addCustomSeText.setText(addCustomSeString);

        cancelButton.setOnClickListener(
                new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        handleBackPressed();
                    }
                });

        addSearchEngineButton.setOnClickListener(
                new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        if (!isSearchEngineValidated()) {
                            return;
                        }

                        String keyword =
                                mTitleEdittext
                                        .getText()
                                        .toString()
                                        .toLowerCase(Locale.getDefault());
                        String title = mTitleEdittext.getText().toString();
                        String url = mUrlEdittext.getText().toString();

                        TemplateUrlService braveTemplateUrlService =
                                TemplateUrlServiceFactory.getForProfile(getProfile());
                        if (!(braveTemplateUrlService instanceof BraveTemplateUrlService)) {
                            return;
                        }

                        BraveTemplateUrlService templateUrlService =
                                (BraveTemplateUrlService) braveTemplateUrlService;

                        if (mIsEditMode) {
                            boolean isUpdated =
                                    templateUrlService.updateSearchEngine(
                                            searchEngineKeyword, title, keyword, url);
                            if (isUpdated) {
                                CustomSearchEnginesUtil.removeCustomSearchEngine(
                                        searchEngineKeyword);
                                CustomSearchEnginesUtil.addCustomSearchEngine(keyword);
                                handleBackPressed();
                            } else {
                                Toast.makeText(
                                                getActivity(),
                                                "Failed to update search engine",
                                                Toast.LENGTH_SHORT)
                                        .show();
                            }
                            return;
                        }

                        if (CustomSearchEnginesUtil.isCustomSearchEngineAdded(keyword)) {
                            Toast.makeText(
                                            getActivity(),
                                            "Search engine is already added",
                                            Toast.LENGTH_SHORT)
                                    .show();
                            return;
                        }

                        if (templateUrlService.addSearchEngine(title, keyword, url)) {
                            CustomSearchEnginesUtil.addCustomSearchEngine(keyword);
                            handleBackPressed();
                        } else {
                            Toast.makeText(
                                            getActivity(),
                                            "Failed to add search engine",
                                            Toast.LENGTH_SHORT)
                                    .show();
                        }
                    }
                });

        requireActivity()
                .getOnBackPressedDispatcher()
                .addCallback(
                        getViewLifecycleOwner(),
                        new OnBackPressedCallback(true) {
                            @Override
                            public void handleOnBackPressed() {
                                handleBackPressed();
                            }
                        });

        return rootView;
    }

    private void handleBackPressed() {
        if (getParentFragmentManager().getBackStackEntryCount() > 0) {
            getParentFragmentManager().popBackStack();
        } else {
            requireActivity().finish(); // Close activity if no fragments left
        }
    }

    private TemplateUrl getTemplateUrlByKeyword(String keyword) {
        TemplateUrlService templateUrlService =
                TemplateUrlServiceFactory.getForProfile(getProfile());
        if (templateUrlService != null) {
            List<TemplateUrl> templateUrls = templateUrlService.getTemplateUrls();
            for (int index = 0; index < templateUrls.size(); ++index) {
                TemplateUrl templateUrl = templateUrls.get(index);
                Log.e(
                        "brave_search",
                        "AddCustomSearchEnginePreferenceFragment : 4 " + templateUrl.getKeyword());
                if (templateUrl.getKeyword().equals(keyword)) {
                    return templateUrl;
                }
            }
        }
        return null;
    }

    private boolean isSearchEngineValidated() {
        boolean isValidated = true;
        if (mTitleEdittext.getText().toString().isEmpty()) {
            isValidated = false;
            mTitleLayout.setError(
                    getResources().getString(R.string.add_custom_search_engine_title_error));
        }
        if (mUrlEdittext.getText().toString().isEmpty()) {
            isValidated = false;
            mUrlLayout.setError(
                    getResources().getString(R.string.add_custom_search_engine_url_error));
        }
        return isValidated;
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
