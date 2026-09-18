/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.quick_search_engines.settings;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.recyclerview.widget.ItemTouchHelper;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.google.android.material.materialswitch.MaterialSwitch;

import org.chromium.base.supplier.MonotonicObservableSupplier;
import org.chromium.base.supplier.ObservableSuppliers;
import org.chromium.base.supplier.SettableMonotonicObservableSupplier;
import org.chromium.brave.browser.quick_search_engines.ItemTouchHelperCallback;
import org.chromium.brave.browser.quick_search_engines.R;
import org.chromium.brave.browser.quick_search_engines.utils.QuickSearchEnginesUtil;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.search_engines.TemplateUrlServiceFactory;
import org.chromium.chrome.browser.settings.ChromeBaseSettingsFragment;
import org.chromium.components.browser_ui.settings.search.BaseSearchIndexProvider;

import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

public class QuickSearchEnginesFragment extends ChromeBaseSettingsFragment
        implements QuickSearchEnginesCallback {
    private RecyclerView mRecyclerView;
    private QuickSearchEnginesAdapter mQuickSearchEnginesAdapter;

    private final SettableMonotonicObservableSupplier<String> mPageTitle =
            ObservableSuppliers.createMonotonic();

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mPageTitle.set(getString(R.string.quick_search_engines));
    }

    // This screen uses a custom layout (R.layout.fragment_quick_search) instead of the preference
    // framework, so there is no PreferenceScreen to build here.
    @Override
    public void onCreatePreferences(Bundle bundle, String rootKey) {
        /* Not used. */
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.fragment_quick_search, container, false);

        LinearLayout quickSearchOptionsLayout = view.findViewById(R.id.quick_search_options_layout);

        // Quick search feature layout
        LinearLayout quickSearchFeatureLayout = view.findViewById(R.id.quick_search_feature_layout);
        TextView quickSearchFeatureText =
                quickSearchFeatureLayout.findViewById(R.id.search_engine_text);
        quickSearchFeatureText.setText(getString(R.string.show_quick_search_bar));
        MaterialSwitch quickSearchFeatureSwitch =
                quickSearchFeatureLayout.findViewById(R.id.search_engine_switch);
        quickSearchFeatureLayout.setOnClickListener(
                v -> {
                    boolean isChecked = quickSearchFeatureSwitch.isChecked();
                    quickSearchFeatureSwitch.setChecked(!isChecked);
                });
        quickSearchFeatureLayout.findViewById(R.id.search_engine_logo).setVisibility(View.GONE);
        quickSearchFeatureSwitch.setChecked(QuickSearchEnginesUtil.getQuickSearchEnginesFeature());
        quickSearchFeatureSwitch.setOnCheckedChangeListener(
                (buttonView, isChecked) -> {
                    quickSearchOptionsLayout.setVisibility(isChecked ? View.VISIBLE : View.GONE);
                    QuickSearchEnginesUtil.setQuickSearchEnginesFeature(isChecked);
                });

        quickSearchOptionsLayout.setVisibility(
                QuickSearchEnginesUtil.getQuickSearchEnginesFeature() ? View.VISIBLE : View.GONE);

        mRecyclerView = view.findViewById(R.id.quick_search_settings_recyclerview);
        LinearLayoutManager linearLayoutManager =
                new LinearLayoutManager(getActivity(), LinearLayoutManager.VERTICAL, false);
        mRecyclerView.setLayoutManager(linearLayoutManager);
        return view;
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        final Profile profile = getProfile();
        TemplateUrlServiceFactory.getForProfile(profile)
                .runWhenLoaded(
                        () -> {
                            if (isRemoving() || isDetached()) return;

                            List<QuickSearchEnginesModel> quickSearchEngines =
                                    QuickSearchEnginesUtil.getQuickSearchEnginesForSettings(
                                            profile);
                            setRecyclerViewData(quickSearchEngines);
                        });
    }

    @Override
    public MonotonicObservableSupplier<String> getPageTitle() {
        return mPageTitle;
    }

    @Override
    public @AnimationType int getAnimationType() {
        return AnimationType.PROPERTY;
    }

    private void setRecyclerViewData(List<QuickSearchEnginesModel> searchEngines) {
        mQuickSearchEnginesAdapter = new QuickSearchEnginesAdapter(searchEngines, this);
        mRecyclerView.setAdapter(mQuickSearchEnginesAdapter);
        ItemTouchHelper.Callback callback = new ItemTouchHelperCallback(mQuickSearchEnginesAdapter);
        new ItemTouchHelper(callback).attachToRecyclerView(mRecyclerView);
    }

    // QuickSearchCallback
    @Override
    public void onSearchEngineClick(int position, QuickSearchEnginesModel quickSearchEnginesModel) {
        updateQuickSearchEnginesInPref(quickSearchEnginesModel);
    }

    private void updateQuickSearchEnginesInPref(QuickSearchEnginesModel quickSearchEnginesModel) {
        Map<String, QuickSearchEnginesModel> searchEnginesMap =
                QuickSearchEnginesUtil.getQuickSearchEnginesFromPref();
        searchEnginesMap.put(quickSearchEnginesModel.getKeyword(), quickSearchEnginesModel);
        QuickSearchEnginesUtil.saveSearchEnginesIntoPref(searchEnginesMap);
    }

    // Persist the new order as soon as the dragged engine is dropped.
    @Override
    public void onSearchEnginesReordered(List<QuickSearchEnginesModel> quickSearchEngines) {
        Map<String, QuickSearchEnginesModel> searchEnginesMap = new LinkedHashMap<>();
        for (QuickSearchEnginesModel quickSearchEnginesModel : quickSearchEngines) {
            searchEnginesMap.put(quickSearchEnginesModel.getKeyword(), quickSearchEnginesModel);
        }
        QuickSearchEnginesUtil.saveSearchEnginesIntoPref(searchEnginesMap);
    }

    @Override
    public void loadSearchEngineLogo(
            ImageView logoView, QuickSearchEnginesModel quickSearchEnginesModel) {
        QuickSearchEnginesUtil.loadSearchEngineLogo(
                getProfile(), logoView, quickSearchEnginesModel.getKeyword());
    }

    // This fragment displays a dynamic RecyclerView of search engines; there are no static
    // preferences to index.
    public static final BaseSearchIndexProvider SEARCH_INDEX_DATA_PROVIDER =
            new BaseSearchIndexProvider(
                    QuickSearchEnginesFragment.class.getName(),
                    BaseSearchIndexProvider.INDEX_OPT_OUT);
}
