/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.quick_search_engines.settings;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CompoundButton;
import android.widget.ImageView;
import android.widget.LinearLayout;

import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.google.android.material.materialswitch.MaterialSwitch;

import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.ObservableSupplierImpl;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.search_engines.TemplateUrlServiceFactory;
import org.chromium.chrome.browser.search_engines.settings.SearchEngineAdapter;
import org.chromium.chrome.browser.settings.BravePreferenceFragment;
import org.chromium.chrome.browser.util.ImageUtils;
import org.chromium.components.search_engines.TemplateUrl;
import org.chromium.components.search_engines.TemplateUrlService;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

public class QuickSearchFragment extends BravePreferenceFragment
        implements TemplateUrlService.LoadListener, QuickSearchCallback {
    private RecyclerView mRecyclerView;
    private QuickSearchAdapter mAdapter;
    private boolean mHasLoadObserver;

    private MenuItem mSaveItem;

    private TemplateUrlService mTemplateUrlService;

    private final ObservableSupplierImpl<String> mPageTitle = new ObservableSupplierImpl<>();

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mPageTitle.set(getString(R.string.quick_search_engines));
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.fragment_quick_search, container, false);

        LinearLayout quickSearchOptionsLayout = view.findViewById(R.id.quick_search_options_layout);
        LinearLayout quickSearchFeatureLayout = view.findViewById(R.id.quick_search_feature_layout);
        MaterialSwitch quickSearchFeatureSwitch =
                view.findViewById(R.id.quick_search_feature_switch);
        quickSearchFeatureLayout.setOnClickListener(
                new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        boolean isChecked = quickSearchFeatureSwitch.isChecked();
                        quickSearchFeatureSwitch.setChecked(!isChecked);
                    }
                });
        quickSearchFeatureSwitch.setChecked(QuickSearchEnginesUtil.getQuickSearchEnginesFeature());
        quickSearchOptionsLayout.setVisibility(
                QuickSearchEnginesUtil.getQuickSearchEnginesFeature() ? View.VISIBLE : View.GONE);
        quickSearchFeatureSwitch.setOnCheckedChangeListener(
                new CompoundButton.OnCheckedChangeListener() {
                    @Override
                    public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                        quickSearchOptionsLayout.setVisibility(
                                isChecked ? View.VISIBLE : View.GONE);
                        QuickSearchEnginesUtil.setQuickSearchEnginesFeature(isChecked);
                    }
                });

        mRecyclerView = (RecyclerView) view.findViewById(R.id.quick_search_settings_recyclerview);
        LinearLayoutManager linearLayoutManager =
                new LinearLayoutManager(getActivity(), LinearLayoutManager.VERTICAL, false);
        mRecyclerView.setLayoutManager(linearLayoutManager);
        setHasOptionsMenu(true);
        return view;
    }

    @Override
    public void onCreateOptionsMenu(Menu menu, MenuInflater inflater) {
        inflater.inflate(R.menu.quick_search_menu, menu);
        mSaveItem = menu.findItem(R.id.action_save);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item.getItemId() == R.id.action_save) {
            if (mAdapter != null
                    && mAdapter.getQuickSearchEngines() != null
                    && mAdapter.getQuickSearchEngines().size() > 0) {
                Map<String, QuickSearchEngineModel> searchEnginesMap =
                        new LinkedHashMap<String, QuickSearchEngineModel>();
                for (QuickSearchEngineModel quickSearchEngineModel :
                        mAdapter.getQuickSearchEngines()) {
                    searchEnginesMap.put(
                            quickSearchEngineModel.getKeyword(), quickSearchEngineModel);
                }
                QuickSearchEnginesUtil.saveSearchEngines(searchEnginesMap);
                mAdapter.setEditMode(false);
                saveMenuVisibility();
            }
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
        refreshData();
    }

    @Override
    public ObservableSupplier<String> getPageTitle() {
        return mPageTitle;
    }

    private void refreshData() {
        mTemplateUrlService = TemplateUrlServiceFactory.getForProfile(getProfile());
        if (!mTemplateUrlService.isLoaded()) {
            mHasLoadObserver = true;
            mTemplateUrlService.registerLoadListener(this);
            mTemplateUrlService.load();
            return; // Flow continues in onTemplateUrlServiceLoaded below.
        }

        List<TemplateUrl> templateUrls = mTemplateUrlService.getTemplateUrls();
        TemplateUrl defaultSearchEngineTemplateUrl =
                mTemplateUrlService.getDefaultSearchEngineTemplateUrl();
        SearchEngineAdapter.sortAndFilterUnnecessaryTemplateUrl(
                templateUrls,
                defaultSearchEngineTemplateUrl,
                mTemplateUrlService.isEeaChoiceCountry());
        List<QuickSearchEngineModel> searchEngines = new ArrayList<>();
        if (QuickSearchEnginesUtil.getSearchEngines() != null) {
            Map<String, QuickSearchEngineModel> searchEnginesMap =
                    QuickSearchEnginesUtil.getSearchEngines();
            for (int i = 0; i < templateUrls.size(); i++) {
                TemplateUrl templateUrl = templateUrls.get(i);
                if (searchEnginesMap.containsKey(templateUrl.getKeyword())) {
                    QuickSearchEngineModel quickSearchEngineModel =
                            searchEnginesMap.get(templateUrl.getKeyword());
                    searchEngines.add(quickSearchEngineModel.getPosition(), quickSearchEngineModel);
                }
            }
        } else {
            Map<String, QuickSearchEngineModel> searchEnginesMap =
                    new LinkedHashMap<String, QuickSearchEngineModel>();
            for (int i = 0; i < templateUrls.size(); i++) {
                TemplateUrl templateUrl = templateUrls.get(i);
                QuickSearchEngineModel quickSearchEngineModel =
                        new QuickSearchEngineModel(
                                templateUrl.getShortName(),
                                templateUrl.getKeyword(),
                                templateUrl.getURL(),
                                true,
                                i);
                searchEngines.add(quickSearchEngineModel);
                searchEnginesMap.put(templateUrl.getKeyword(), quickSearchEngineModel);
            }
            QuickSearchEnginesUtil.saveSearchEngines(searchEnginesMap);
        }
        setRecyclerViewData(searchEngines);
    }

    private void setRecyclerViewData(List<QuickSearchEngineModel> searchEngines) {
        mAdapter = new QuickSearchAdapter(getActivity(), searchEngines, this);
        mRecyclerView.setAdapter(mAdapter);
    }

    @Override
    public void onDestroy() {
        if (mHasLoadObserver) {
            TemplateUrlServiceFactory.getForProfile(getProfile()).unregisterLoadListener(this);
            mHasLoadObserver = false;
        }
        super.onDestroy();
    }

    // TemplateUrlService.LoadListener
    @Override
    public void onTemplateUrlServiceLoaded() {
        TemplateUrlServiceFactory.getForProfile(getProfile()).unregisterLoadListener(this);
        mHasLoadObserver = false;
        refreshData();
    }

    // QuickSearchCallback
    @Override
    public void onSearchEngineClick(QuickSearchEngineModel quickSearchEngineModel) {
        Map<String, QuickSearchEngineModel> searchEnginesMap =
                QuickSearchEnginesUtil.getSearchEngines();
        searchEnginesMap.put(quickSearchEngineModel.getKeyword(), quickSearchEngineModel);
        QuickSearchEnginesUtil.saveSearchEngines(searchEnginesMap);
    }

    @Override
    public void onSearchEngineLongClick() {
        saveMenuVisibility();
    }

    private void saveMenuVisibility() {
        if (mSaveItem != null && mAdapter != null) {
            mSaveItem.setVisible(mAdapter.isEditMode());
        }
    }

    @Override
    public void loadSearchEngineLogo(
            ImageView logoView, QuickSearchEngineModel quickSearchEngineModel) {
        ImageUtils.loadSearchEngineLogo(
                getProfile(), logoView, quickSearchEngineModel.getKeyword());
    }
}
