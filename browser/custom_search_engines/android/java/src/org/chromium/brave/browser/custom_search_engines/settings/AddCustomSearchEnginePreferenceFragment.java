/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.custom_search_engines.settings;

import android.graphics.Typeface;
import android.os.Bundle;
import android.text.Editable;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.TextWatcher;
import android.text.style.StyleSpan;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

import androidx.activity.OnBackPressedCallback;

import com.google.android.material.textfield.TextInputEditText;
import com.google.android.material.textfield.TextInputLayout;

import org.chromium.base.supplier.MonotonicObservableSupplier;
import org.chromium.base.supplier.ObservableSuppliers;
import org.chromium.base.supplier.SettableMonotonicObservableSupplier;
import org.chromium.brave.browser.custom_search_engines.CustomSearchEnginesManager;
import org.chromium.brave.browser.custom_search_engines.R;
import org.chromium.build.annotations.MonotonicNonNull;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.search_engines.TemplateUrlServiceFactory;
import org.chromium.chrome.browser.settings.ChromeBaseSettingsFragment;
import org.chromium.components.browser_ui.settings.search.BaseSearchIndexProvider;
import org.chromium.components.search_engines.TemplateUrl;
import org.chromium.components.search_engines.TemplateUrlService;
import org.chromium.ui.widget.Toast;
import org.chromium.url.GURL;

import java.util.List;
import java.util.Locale;

@NullMarked
public class AddCustomSearchEnginePreferenceFragment extends ChromeBaseSettingsFragment {
    private @Nullable TextInputEditText mTitleEdittext;

    private @Nullable TextInputEditText mUrlEdittext;

    private @Nullable TextInputLayout mUrlLayout;

    private @Nullable Button mAddSearchEngineButton;
    private @Nullable Button mCancelButton;
    private boolean mIsEditMode;

    private @Nullable String mSearchEngineKeywordForEdit;

    private @MonotonicNonNull CustomSearchEnginesManager mCustomSearchEnginesManager;

    private final SettableMonotonicObservableSupplier<String> mPageTitle =
            ObservableSuppliers.createMonotonic();

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mPageTitle.set(getString(R.string.add_custom_search_engine));
        mCustomSearchEnginesManager = CustomSearchEnginesManager.getInstance();
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater,
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
        Bundle args = getArguments();
        if (args == null) {
            return;
        }
        mSearchEngineKeywordForEdit = args.getString(CustomSearchEnginesManager.KEYWORD);
        if (mSearchEngineKeywordForEdit == null) {
            return;
        }

        String keywordForEdit = mSearchEngineKeywordForEdit;
        TemplateUrlServiceFactory.getForProfile(getProfile())
                .runWhenLoaded(
                        () -> {
                            if (!isAdded() || isDetached() || getActivity() == null) {
                                return;
                            }
                            if (keywordForEdit == null) {
                                return;
                            }
                            TemplateUrl templateUrl = getTemplateUrlByKeyword(keywordForEdit);
                            if (templateUrl != null) {
                                populateFields(templateUrl);
                                mIsEditMode = true;
                            }
                        });

        mPageTitle.set(getString(R.string.edit_custom_search_engine));
        if (mAddSearchEngineButton != null) {
            mAddSearchEngineButton.setText(getString(R.string.save_changes_action_text));
        }
    }

    private void populateFields(TemplateUrl templateUrl) {
        if (mTitleEdittext == null || mUrlEdittext == null || mAddSearchEngineButton == null) {
            return;
        }
        mTitleEdittext.setText(templateUrl.getShortName());
        String queryReplacedUrl = templateUrl.getURL().replace("{searchTerms}", "%s");
        mUrlEdittext.setText(queryReplacedUrl);
        mAddSearchEngineButton.setText(getString(R.string.save_changes_action_text));
    }

    private void initViews(View rootView) {
        mTitleEdittext = (TextInputEditText) rootView.findViewById(R.id.title_edittext);

        mUrlEdittext = (TextInputEditText) rootView.findViewById(R.id.url_edittext);
        mUrlLayout = (TextInputLayout) rootView.findViewById(R.id.url_layout);

        TextView addCustomSeText = (TextView) rootView.findViewById(R.id.add_custom_se_text);
        String text1 = getResources().getString(R.string.add_custom_search_engine_text1);
        String text2 = getResources().getString(R.string.add_custom_search_engine_text2);
        SpannableStringBuilder ssb = new SpannableStringBuilder();
        ssb.append(text1);
        // Bold "%s." at the end of text1
        int t1BoldStart = text1.indexOf("%s");
        if (t1BoldStart >= 0) {
            ssb.setSpan(
                    new StyleSpan(Typeface.BOLD),
                    t1BoldStart,
                    Math.min(t1BoldStart + 3, text1.length()),
                    Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        }
        ssb.append("\n\n");
        int text2Offset = ssb.length();
        ssb.append(text2);
        // Bold the URL (from "https://" through "%s.") in text2
        int urlStart = text2.indexOf("https://");
        int urlEnd = text2.indexOf("%s.", urlStart >= 0 ? urlStart : 0);
        if (urlStart >= 0 && urlEnd >= 0) {
            ssb.setSpan(
                    new StyleSpan(Typeface.BOLD),
                    text2Offset + urlStart,
                    text2Offset + urlEnd + 3,
                    Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        }
        addCustomSeText.setText(ssb);

        initTextWatchers();

        initButtons(rootView);
    }

    private void initButtons(View rootView) {
        mCancelButton = (Button) rootView.findViewById(R.id.cancel_button);
        mAddSearchEngineButton = (Button) rootView.findViewById(R.id.add_search_engine_button);
        if (mAddSearchEngineButton != null) {
            mAddSearchEngineButton.setEnabled(mIsEditMode);
        }

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
                        if (mTitleEdittext == null
                                || mUrlEdittext == null
                                || mCustomSearchEnginesManager == null) {
                            return;
                        }
                        Editable titleEditable = mTitleEdittext.getText();
                        Editable urlEditable = mUrlEdittext.getText();
                        if (titleEditable == null || urlEditable == null) {
                            return;
                        }
                        String title = titleEditable.toString();
                        String keyword = title.toLowerCase(Locale.getDefault());
                        String url = urlEditable.toString();

                        if (!isUrlValid(url)) {
                            return;
                        }

                        TemplateUrlService templateUrlService =
                                TemplateUrlServiceFactory.getForProfile(getProfile());

                        if (mIsEditMode && mSearchEngineKeywordForEdit != null) {
                            handleSearchEngineUpdate(
                                    templateUrlService,
                                    mSearchEngineKeywordForEdit,
                                    title,
                                    keyword,
                                    url);
                        } else if (!mIsEditMode) {
                            handleSearchEngineAdd(templateUrlService, title, keyword, url);
                        }
                    }
                });
    }

    private void handleSearchEngineUpdate(
            TemplateUrlService templateUrlService,
            String searchEngineKeyword,
            String title,
            String keyword,
            String url) {
        if (mCustomSearchEnginesManager == null) {
            return;
        }
        // TODO(https://github.com/brave/brave-browser/issues/21837): implement the actual update of
        // the custom search engine in template url
        // service.
        boolean isUpdated = templateUrlService != null;
        if (isUpdated) {
            mCustomSearchEnginesManager.updateCustomSearchEngine(
                    searchEngineKeyword, title, keyword, url);
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
            TemplateUrlService templateUrlService, String title, String keyword, String url) {
        if (mCustomSearchEnginesManager == null) {
            return;
        }
        if (mCustomSearchEnginesManager.isCustomSearchEngineAdded(keyword)) {
            Toast.makeText(
                            getActivity(),
                            getString(R.string.search_engine_already_exists_error),
                            Toast.LENGTH_SHORT)
                    .show();
            return;
        }

        // TODO(https://github.com/brave/brave-browser/issues/21837): implement the actual addition
        // of the custom search engine to template
        // url service.
        boolean isAdded = templateUrlService != null && !url.isEmpty() && !title.isEmpty();
        if (isAdded) {
            mCustomSearchEnginesManager.addCustomSearchEngine(keyword);
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

        Editable titleEditable = mTitleEdittext.getText();
        Editable urlEditable = mUrlEdittext.getText();
        String title = titleEditable != null ? titleEditable.toString().trim() : "";
        String url = urlEditable != null ? urlEditable.toString().trim() : "";
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
                        // no-op, but required by TextWatcher interface
                    }

                    @Override
                    public void onTextChanged(CharSequence s, int start, int before, int count) {
                        if (s != null) {
                            checkFields();
                        }
                    }

                    @Override
                    public void afterTextChanged(Editable s) {
                        // no-op, but required by TextWatcher interface
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

    private @Nullable TemplateUrl getTemplateUrlByKeyword(String keyword) {
        TemplateUrlService templateUrlService =
                TemplateUrlServiceFactory.getForProfile(getProfile());
        List<TemplateUrl> templateUrls = templateUrlService.getTemplateUrls();
        for (int index = 0; index < templateUrls.size(); ++index) {
            TemplateUrl templateUrl = templateUrls.get(index);
            if (templateUrl.getKeyword().equals(keyword)) {
                return templateUrl;
            }
        }
        return null;
    }

    private boolean isUrlValid(String url) {
        GURL gurl = new GURL(url);
        boolean isValid = !GURL.isEmptyOrInvalid(gurl) && url.contains("%s");

        if (!isValid && mUrlLayout != null) {
            mUrlLayout.setError(
                    getResources().getString(R.string.add_custom_search_engine_url_error));
            return false;
        }

        return isValid;
    }

    @Override
    public void onCreatePreferences(@Nullable Bundle bundle, @Nullable String s) {
        setHasOptionsMenu(true);
    }

    @Override
    public MonotonicObservableSupplier<String> getPageTitle() {
        return mPageTitle;
    }

    @Override
    public @AnimationType int getAnimationType() {
        return AnimationType.PROPERTY;
    }

    // Custom form layout only; no static preference XML to index.
    public static final BaseSearchIndexProvider SEARCH_INDEX_DATA_PROVIDER =
            new BaseSearchIndexProvider(
                    AddCustomSearchEnginePreferenceFragment.class.getName(),
                    BaseSearchIndexProvider.INDEX_OPT_OUT);
}
