/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.search_engines.settings;

import android.app.Activity;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
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

    private Button mAddSearchEngineButton;
    private Button mCancelButton;
    private boolean mIsEditMode;

    private String mSearchEngineKeywordForEdit;

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
        View rootView =
                inflater.inflate(
                        R.layout.add_custom_search_engine_preference_layout, container, false);

        initViews(rootView);
        updateToolbarTitle();

        OnBackPressedCallback callback =
                new OnBackPressedCallback(true) {
                    @Override
                    public void handleOnBackPressed() {
                        handleBackPressed();
                    }
                };
        requireActivity()
                .getOnBackPressedDispatcher()
                .addCallback(getViewLifecycleOwner(), callback);

        return rootView;
    }

    private void updateToolbarTitle() {
        mSearchEngineKeywordForEdit = getArguments().getString(CustomSearchEnginesUtil.KEYWORD);
        if (mSearchEngineKeywordForEdit == null) {
            return;
        }

        TemplateUrlServiceFactory.getForProfile(getProfile())
                .runWhenLoaded(
                        () -> {
                            TemplateUrl templateUrl =
                                    getTemplateUrlByKeyword(mSearchEngineKeywordForEdit);
                            if (templateUrl != null) {
                                populateFields(templateUrl);
                                mIsEditMode = true;
                            }
                        });

        updateActionBarTitle();
    }

    private void populateFields(TemplateUrl templateUrl) {
        mTitleEdittext.setText(templateUrl.getShortName());
        String queryReplacedUrl = templateUrl.getURL().replace("{searchTerms}", "%s");
        mUrlEdittext.setText(queryReplacedUrl);
        mAddSearchEngineButton.setText(getString(R.string.save_changes_action_text));
    }

    private void updateActionBarTitle() {
        Activity activity = getActivity();
        if (activity != null) {
            Toolbar actionBar = activity.findViewById(R.id.action_bar);
            if (actionBar != null) {
                actionBar.setTitle(getString(R.string.edit_custom_search_engine));
            }
        }
    }

    private void initViews(View rootView) {
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

        initTextWatchers();

        initButtons(rootView);
    }

    private void initButtons(View rootView) {
        mCancelButton = (Button) rootView.findViewById(R.id.cancel_button);
        mAddSearchEngineButton = (Button) rootView.findViewById(R.id.add_search_engine_button);
        mAddSearchEngineButton.setEnabled(mIsEditMode);

        handleClickListeners();
    }

    private void handleClickListeners() {
        if (mCancelButton == null || mAddSearchEngineButton == null) {
            return;
        }

        mCancelButton.setOnClickListener(
                new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        handleBackPressed();
                    }
                });

        mAddSearchEngineButton.setOnClickListener(
                new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        String title = mTitleEdittext.getText().toString();
                        String keyword = title.toLowerCase(Locale.getDefault());
                        String url = mUrlEdittext.getText().toString();

                        if (!isUrlValid(url)) {
                            return;
                        }

                        TemplateUrlService braveTemplateUrlService =
                                TemplateUrlServiceFactory.getForProfile(getProfile());
                        if (!(braveTemplateUrlService instanceof BraveTemplateUrlService)) {
                            return;
                        }

                        BraveTemplateUrlService templateUrlService =
                                (BraveTemplateUrlService) braveTemplateUrlService;

                        if (mIsEditMode) {
                            handleSearchEngineUpdate(
                                    templateUrlService,
                                    mSearchEngineKeywordForEdit,
                                    title,
                                    keyword,
                                    url);
                        } else {
                            handleSearchEngineAdd(templateUrlService, title, keyword, url);
                        }
                    }
                });
    }

    private void handleSearchEngineUpdate(
            BraveTemplateUrlService templateUrlService,
            String searchEngineKeyword,
            String title,
            String keyword,
            String url) {
        String queryReplacedUrl = url.replace("%s", "{searchTerms}");
        boolean isUpdated =
                templateUrlService.updateSearchEngine(
                        searchEngineKeyword, title, keyword, queryReplacedUrl);
        if (isUpdated) {
            CustomSearchEnginesUtil.removeCustomSearchEngine(searchEngineKeyword);
            CustomSearchEnginesUtil.addCustomSearchEngine(keyword);
            handleBackPressed();
        } else {
            Toast.makeText(
                            getActivity(),
                            getString(R.string.failed_to_update_search_engine),
                            Toast.LENGTH_SHORT)
                    .show();
        }
    }

    private void handleSearchEngineAdd(
            BraveTemplateUrlService templateUrlService, String title, String keyword, String url) {
        String queryReplacedUrl = url.replace("%s", "{searchTerms}");
        if (CustomSearchEnginesUtil.isCustomSearchEngineAdded(keyword)) {
            Toast.makeText(
                            getActivity(),
                            getString(R.string.search_engine_already_exists_error),
                            Toast.LENGTH_SHORT)
                    .show();
            return;
        }

        if (templateUrlService.addSearchEngine(title, keyword, queryReplacedUrl)) {
            CustomSearchEnginesUtil.addCustomSearchEngine(keyword);
            handleBackPressed();
        } else {
            Toast.makeText(
                            getActivity(),
                            getString(R.string.failed_to_add_search_engine),
                            Toast.LENGTH_SHORT)
                    .show();
        }
    }

    private void checkFields() {
        if (mTitleEdittext == null || mUrlEdittext == null || mAddSearchEngineButton == null) {
            return;
        }

        String title = mTitleEdittext.getText().toString().trim();
        String url = mUrlEdittext.getText().toString().trim();
        mAddSearchEngineButton.setEnabled(!title.isEmpty() && !url.isEmpty());
    }

    private void initTextWatchers() {
        if (mTitleEdittext == null || mUrlEdittext == null) {
            return;
        }

        TextWatcher textWatcher =
                new TextWatcher() {
                    @Override
                    public void beforeTextChanged(CharSequence s, int start, int count, int after) {
                        // Not used
                    }

                    @Override
                    public void onTextChanged(CharSequence s, int start, int before, int count) {
                        if (s != null) {
                            checkFields();
                        }
                    }

                    @Override
                    public void afterTextChanged(Editable s) {
                        // Not used
                    }
                };

        mTitleEdittext.addTextChangedListener(textWatcher);
        mUrlEdittext.addTextChangedListener(textWatcher);
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
                if (templateUrl.getKeyword().equals(keyword)) {
                    return templateUrl;
                }
            }
        }
        return null;
    }

    private boolean isUrlValid(String url) {
        boolean isValid = !url.isEmpty() && CustomSearchEnginesUtil.isSearchQuery(url);

        if (!isValid) {
            mUrlLayout.setError(
                    getResources().getString(R.string.add_custom_search_engine_url_error));
            return false;
        }

        return true;
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
