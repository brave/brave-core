/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.quick_search_engines.settings;

import android.content.Context;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.search_engines.TemplateUrlServiceFactory;
import org.chromium.chrome.browser.search_engines.settings.SearchEngineAdapter;
import org.chromium.chrome.browser.settings.BravePreferenceFragment;
import org.chromium.components.search_engines.TemplateUrl;
import org.chromium.components.search_engines.TemplateUrlService;

import java.util.ArrayList;
import java.util.List;

public class QuickSearchFragment extends BravePreferenceFragment {
    private RecyclerView mRecyclerView;
    private QuickSearchAdapter mAdapter;

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.fragment_quick_search, container, false);
        mRecyclerView = (RecyclerView) view.findViewById(R.id.quick_search_settings_recyclerview);
        LinearLayoutManager linearLayoutManager =
                new LinearLayoutManager(getActivity(), LinearLayoutManager.VERTICAL, false);
        mRecyclerView.setLayoutManager(linearLayoutManager);
        return view;
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        if (getActivity() != null) {
            getActivity().setTitle(R.string.quick_search_engines);
        }
        super.onActivityCreated(savedInstanceState);
        refreshData();
    }

    @Override
    public void onAttach(@NonNull Context context) {
        super.onAttach(context);
    }

    private void refreshData() {
        TemplateUrlService templateUrlService =
                TemplateUrlServiceFactory.getForProfile(getProfile());
        // if (!templateUrlService.isLoaded()) {
        //     mHasLoadObserver = true;
        //     templateUrlService.registerLoadListener(this);
        //     templateUrlService.load();
        //     return; // Flow continues in onTemplateUrlServiceLoaded below.
        // }

        List<TemplateUrl> templateUrls = templateUrlService.getTemplateUrls();
        TemplateUrl defaultSearchEngineTemplateUrl =
                templateUrlService.getDefaultSearchEngineTemplateUrl();
        SearchEngineAdapter.sortAndFilterUnnecessaryTemplateUrl(
                templateUrls,
                defaultSearchEngineTemplateUrl,
                templateUrlService.isEeaChoiceCountry(),
                templateUrlService.shouldShowUpdatedSettings());

        List<TemplateUrl> searchEngines = new ArrayList<>();
        for (int i = 0; i < templateUrls.size(); i++) {
            TemplateUrl templateUrl = templateUrls.get(i);
            searchEngines.add(templateUrl);
        }
        mAdapter = new QuickSearchAdapter(getActivity(), searchEngines);
        mRecyclerView.setAdapter(mAdapter);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
    }
}
