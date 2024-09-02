/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.webkit.URLUtil;

import androidx.appcompat.widget.SearchView;
import androidx.appcompat.widget.Toolbar;
import androidx.core.content.ContextCompat;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import androidx.recyclerview.widget.SimpleItemAnimator;

import com.bumptech.glide.Glide;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.ObservableSupplierImpl;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;
import org.chromium.brave_news.mojom.BraveNewsController;
import org.chromium.brave_news.mojom.Channel;
import org.chromium.brave_news.mojom.FeedSearchResultItem;
import org.chromium.brave_news.mojom.Publisher;
import org.chromium.brave_news.mojom.UserEnabled;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.brave_news.BraveNewsControllerFactory;
import org.chromium.chrome.browser.brave_news.BraveNewsUtils;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.util.BraveConstants;
import org.chromium.components.browser_ui.settings.SearchUtils;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;
import org.chromium.url.mojom.Url;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class BraveNewsPreferencesDetails extends BravePreferenceFragment
        implements BraveNewsPreferencesListener, ConnectionErrorHandler {
    private RecyclerView mRecyclerView;

    private BraveNewsPreferencesTypeAdapter mAdapter;
    private BraveNewsController mBraveNewsController;
    private List<Publisher> mPublisherList;
    private String mBraveNewsPreferencesType;
    private String mSearch = "";
    private HashMap<String, String> mFeedSearchResultItemFollowMap = new HashMap<>();
    private final ObservableSupplierImpl<String> mPageTitle = new ObservableSupplierImpl<>();

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(R.layout.brave_news_settings_details, container, false);
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        initBraveNewsController();

        mRecyclerView = (RecyclerView) getView().findViewById(R.id.recyclerview);

        mBraveNewsPreferencesType =
                getArguments().getString(BraveConstants.BRAVE_NEWS_PREFERENCES_TYPE);
        setData();
    }

    private void setData() {
        List<Publisher> publisherList = new ArrayList<>();
        List<Channel> channelsList = new ArrayList<>();
        if (mBraveNewsPreferencesType.equalsIgnoreCase(
                BraveNewsPreferencesType.PopularSources.toString())) {
            publisherList = BraveNewsUtils.getPopularSources();
            mPageTitle.set(getString(R.string.popular));
        } else if (mBraveNewsPreferencesType.equalsIgnoreCase(
                BraveNewsPreferencesType.Suggestions.toString())) {
            publisherList = BraveNewsUtils.getSuggestionsPublisherList();
            mPageTitle.set(getString(R.string.suggestions));
        } else if (mBraveNewsPreferencesType.equalsIgnoreCase(
                BraveNewsPreferencesType.Channels.toString())) {
            mPageTitle.set(getString(R.string.channels));
            channelsList = BraveNewsUtils.getChannelList();
        } else if (mBraveNewsPreferencesType.equalsIgnoreCase(
                BraveNewsPreferencesType.Following.toString())) {
            mPageTitle.set(getString(R.string.following));
            publisherList = BraveNewsUtils.getFollowingPublisherList();
            channelsList = BraveNewsUtils.getFollowingChannelList();
        } else if (mBraveNewsPreferencesType.equalsIgnoreCase(
                BraveNewsPreferencesType.Search.toString())) {
            getView().findViewById(R.id.search_divider).setVisibility(View.VISIBLE);

            Toolbar actionBar = getActivity().findViewById(R.id.action_bar);
            actionBar.setContentInsetsAbsolute(0, 0);
            actionBar.setContentInsetStartWithNavigation(0);
        }

        LinearLayoutManager linearLayoutManager =
                new LinearLayoutManager(getActivity(), LinearLayoutManager.VERTICAL, false);
        mRecyclerView.setLayoutManager(linearLayoutManager);
        mAdapter = new BraveNewsPreferencesTypeAdapter(getActivity(), this,
                BraveNewsPreferencesSearchType.Init, mBraveNewsController,
                Glide.with(getActivity()), mBraveNewsPreferencesType, channelsList, publisherList);
        mRecyclerView.setAdapter(mAdapter);

        if (mRecyclerView.getItemAnimator() != null) {
            RecyclerView.ItemAnimator itemAnimator = mRecyclerView.getItemAnimator();
            if (itemAnimator instanceof SimpleItemAnimator) {
                SimpleItemAnimator simpleItemAnimator = (SimpleItemAnimator) itemAnimator;
                simpleItemAnimator.setSupportsChangeAnimations(false);
            }
        }

        Drawable horizontalDivider = ContextCompat.getDrawable(
                getActivity(), R.drawable.brave_news_settings_list_divider);
        mRecyclerView.addItemDecoration(
                new BraveNewsSettingsDividerItemDecoration(horizontalDivider));
    }

    @Override
    public ObservableSupplier<String> getPageTitle() {
        return mPageTitle;
    }

    private void initBraveNewsController() {
        if (mBraveNewsController != null) {
            return;
        }

        mBraveNewsController =
                BraveNewsControllerFactory.getInstance().getBraveNewsController(this);
    }

    @Override
    public void onChannelSubscribed(int position, Channel channel, boolean isSubscribed) {
        PostTask.postTask(TaskTraits.BEST_EFFORT, () -> {
            if (mBraveNewsController != null) {
                newsChangeSource();
                mBraveNewsController.setChannelSubscribed(BraveNewsUtils.getLocale(),
                        channel.channelName, isSubscribed,
                        ((updatedChannel) -> { BraveNewsUtils.setFollowingChannelList(); }));
            }
        });
    }

    @Override
    public void onPublisherPref(String publisherId, int userEnabled) {
        PostTask.postTask(TaskTraits.BEST_EFFORT, () -> {
            if (mBraveNewsController != null) {
                newsChangeSource();
                mBraveNewsController.setPublisherPref(publisherId, userEnabled);
                BraveNewsUtils.setFollowingPublisherList();
            }
        });
    }

    @Override
    public void findFeeds(String url) {
        PostTask.postTask(TaskTraits.BEST_EFFORT, () -> {
            if (mBraveNewsController != null) {
                Url searchUrl = new Url();
                searchUrl.url = url;
                mBraveNewsController.findFeeds(searchUrl, ((results) -> {
                    if (!url.equals(mSearch)) return;

                    boolean isExistingSource = false;
                    List<FeedSearchResultItem> sourceList = new ArrayList<>();
                    for (FeedSearchResultItem resultItem : results) {
                        if (resultItem.feedUrl != null
                                && !BraveNewsUtils.searchPublisherForRss(resultItem.feedUrl.url)) {
                            sourceList.add(resultItem);
                        } else {
                            isExistingSource = true;
                        }
                    }
                    BraveNewsPreferencesSearchType braveNewsPreferencesSearchType;
                    if (sourceList.size() > 0) {
                        braveNewsPreferencesSearchType = BraveNewsPreferencesSearchType.NewSource;
                    } else if (isExistingSource) {
                        braveNewsPreferencesSearchType =
                                BraveNewsPreferencesSearchType.Init; // ExistingSource;
                    } else {
                        braveNewsPreferencesSearchType = BraveNewsPreferencesSearchType.NotFound;
                    }
                    mAdapter.setFindFeeds(sourceList, braveNewsPreferencesSearchType);
                }));
            }
        });
    }

    @Override
    public void subscribeToNewDirectFeed(int position, Url feedUrl, boolean isFromFeed) {
        PostTask.postTask(TaskTraits.BEST_EFFORT, () -> {
            if (mBraveNewsController != null) {
                mBraveNewsController.subscribeToNewDirectFeed(
                        feedUrl, ((isValidFeed, isDuplicate, publishers) -> {
                            if (isValidFeed && publishers != null && publishers.size() > 0) {
                                newsChangeSource();
                                BraveNewsUtils.setPublishers(publishers);
                            }

                            if (publishers != null) {
                                for (Map.Entry<String, Publisher> entry : publishers.entrySet()) {
                                    Publisher publisher = entry.getValue();
                                    if (publisher.feedSource.url.equalsIgnoreCase(feedUrl.url)) {
                                        publisher.userEnabledStatus = UserEnabled.ENABLED;
                                        if (isFromFeed) {
                                            updateFeedSearchResultItem(position,
                                                    publisher.feedSource.url,
                                                    publisher.publisherId);
                                        } else {
                                            mAdapter.notifyItemChanged(position);
                                        }
                                        break;
                                    }
                                }
                            }
                        }));
            }
        });
    }

    @Override
    public void updateFeedSearchResultItem(int position, String url, String publisherId) {
        if (mFeedSearchResultItemFollowMap.containsKey(url)) {
            mFeedSearchResultItemFollowMap.remove(url);
        } else {
            mFeedSearchResultItemFollowMap.put(url, publisherId);
        }
        mAdapter.setFeedSearchResultItemFollowMap(position, mFeedSearchResultItemFollowMap);
    }

    @Override
    public void onCreateOptionsMenu(Menu menu, MenuInflater inflater) {
        MenuItem closeItem = menu.findItem(R.id.close_menu_id);
        if (closeItem != null) {
            closeItem.setVisible(false);
        }
        if (mBraveNewsPreferencesType.equalsIgnoreCase(
                    BraveNewsPreferencesType.Search.toString())) {
            inflater.inflate(R.menu.menu_brave_news_settings_search, menu);

            MenuItem searchItem = menu.findItem(R.id.menu_id_search);
            SearchView searchView = (SearchView) searchItem.getActionView();
            searchView.setMaxWidth(Integer.MAX_VALUE);
            searchView.setQueryHint(getActivity().getString(R.string.brave_news_settings_search));
            SearchUtils.initializeSearchView(searchItem, mSearch, getActivity(), (query) -> {
                boolean queryHasChanged = mSearch == null ? query != null && !query.isEmpty()
                                                          : !mSearch.equals(query);
                mSearch = query;
                if (queryHasChanged && mSearch.length() > 0) {
                    search();
                } else if (mSearch.length() == 0) {
                    mAdapter.notifyItemRangeRemoved(0, mAdapter.getItemCount());
                    mAdapter.setItems(new ArrayList<Channel>(), new ArrayList<Publisher>(), null,
                            BraveNewsPreferencesSearchType.Init, mFeedSearchResultItemFollowMap);
                }
            });
        }
    }

    private void search() {
        List<Channel> channelList = BraveNewsUtils.searchChannel(mSearch);
        List<Publisher> publisherList = BraveNewsUtils.searchPublisher(mSearch);
        String feedUrl = mSearch;
        String searchUrl = null;
        mFeedSearchResultItemFollowMap = new HashMap<>();
        BraveNewsPreferencesSearchType braveNewsPreferencesSearchType =
                BraveNewsPreferencesSearchType.Init;

        if (feedUrl.contains(".")) {
            if (!feedUrl.contains("://")) {
                feedUrl = "https://" + feedUrl;
            }

            if (URLUtil.isValidUrl(feedUrl)) {
                searchUrl = feedUrl;

                braveNewsPreferencesSearchType = BraveNewsPreferencesSearchType.SearchUrl;
            }
        }
        mSearch = searchUrl;
        mAdapter.notifyItemRangeRemoved(0, mAdapter.getItemCount());
        mAdapter.setItems(channelList, publisherList, searchUrl, braveNewsPreferencesSearchType,
                mFeedSearchResultItemFollowMap);
        mRecyclerView.scrollToPosition(0);
    }

    public void newsChangeSource() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_NEWS_CHANGE_SOURCE, true);
    }

    @Override
    public void onConnectionError(MojoException e) {
        if (mBraveNewsController != null) {
            mBraveNewsController.close();
        }
        mBraveNewsController = null;
        initBraveNewsController();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        if (mBraveNewsController != null) {
            mBraveNewsController.close();
        }
    }
}
