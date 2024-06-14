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
import android.widget.ImageView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.search_engines.TemplateUrlServiceFactory;
import org.chromium.chrome.browser.search_engines.settings.SearchEngineAdapter;
import org.chromium.chrome.browser.settings.BravePreferenceFragment;
import org.chromium.chrome.browser.ui.favicon.FaviconUtils;
import org.chromium.components.favicon.LargeIconBridge;
import org.chromium.components.favicon.LargeIconBridge.GoogleFaviconServerCallback;
import org.chromium.components.favicon.LargeIconBridge.LargeIconCallback;
import org.chromium.components.search_engines.TemplateUrl;
import org.chromium.components.search_engines.TemplateUrlService;
import org.chromium.net.NetworkTrafficAnnotationTag;
import org.chromium.url.GURL;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class QuickSearchFragment extends BravePreferenceFragment
        implements TemplateUrlService.LoadListener, QuickSearchCallback {
    private RecyclerView mRecyclerView;
    private QuickSearchAdapter mAdapter;
    private boolean mHasLoadObserver;
    private LargeIconBridge mLargeIconBridge;

    private TemplateUrlService mTemplateUrlService;

    private static final NetworkTrafficAnnotationTag TRAFFIC_ANNOTATION =
            NetworkTrafficAnnotationTag.createComplete(
                    "quick_search_engines_fragment",
                    """
            semantics {
                sender: 'QuickSearchEnginesFragment'
                description: 'Sends a request to a Google server to retrieve the favicon bitmap.'
                trigger:
                    'A request is sent when the user opens search engine settings and Chrome does '
                    'not have a favicon.'
                data: 'Search engine URL and desired icon size.'
                destination: GOOGLE_OWNED_SERVICE
                internal {
                    contacts {
                        email: 'chrome-signin-team@google.com'
                    }
                    contacts {
                        email: 'triploblastic@google.com'
                    }
                }
                user_data {
                    type: NONE
                }
                last_reviewed: '2023-12-04'
            }
            policy {
    cookies_allowed:
        NO policy_exception_justification : 'Not implemented.' setting
            : 'This feature cannot be disabled by settings.'
            }""");

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
        mLargeIconBridge = new LargeIconBridge(getProfile());
        refreshData();
    }

    @Override
    public void onAttach(@NonNull Context context) {
        super.onAttach(context);
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
                mTemplateUrlService.isEeaChoiceCountry(),
                mTemplateUrlService.shouldShowUpdatedSettings());
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
                    new HashMap<String, QuickSearchEngineModel>();
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
        mLargeIconBridge.destroy();
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
        Log.e("quick_search : onSearchEngineClick", quickSearchEngineModel.getShortName());
    }

    @Override
    public void onSearchEngineLongClick() {}

    @Override
    public void loadSearchEngineLogo(
            ImageView logoView, QuickSearchEngineModel quickSearchEngineModel) {
        GURL faviconUrl =
                new GURL(
                        mTemplateUrlService.getSearchEngineUrlFromTemplateUrl(
                                quickSearchEngineModel.getKeyword()));
        // Use a placeholder image while trying to fetch the logo.
        int uiElementSizeInPx =
                getActivity()
                        .getResources()
                        .getDimensionPixelSize(R.dimen.search_engine_favicon_size);
        logoView.setImageBitmap(
                FaviconUtils.createGenericFaviconBitmap(getActivity(), uiElementSizeInPx, null));
        LargeIconCallback onFaviconAvailable =
                (icon, fallbackColor, isFallbackColorDefault, iconType) -> {
                    if (icon != null) {
                        logoView.setImageBitmap(icon);
                    }
                };
        GoogleFaviconServerCallback googleServerCallback =
                (status) -> {
                    // Update the time the icon was last requested to avoid automatic eviction
                    // from cache.
                    mLargeIconBridge.touchIconFromGoogleServer(faviconUrl);
                    // The search engine logo will be fetched from google servers, so the actual
                    // size of the image is controlled by LargeIconService configuration.
                    // minSizePx=1 is used to accept logo of any size.
                    mLargeIconBridge.getLargeIconForUrl(
                            faviconUrl,
                            /* minSizePx= */ 1,
                            /* desiredSizePx= */ uiElementSizeInPx,
                            onFaviconAvailable);
                };
        // If the icon already exists in the cache no network request will be made, but the
        // callback will be triggered nonetheless.
        mLargeIconBridge.getLargeIconOrFallbackStyleFromGoogleServerSkippingLocalCache(
                faviconUrl,
                /* shouldTrimPageUrlPath= */ true,
                TRAFFIC_ANNOTATION,
                googleServerCallback);
    }
}
