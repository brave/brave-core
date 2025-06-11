/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.brave_news;

import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;
import org.chromium.brave_news.mojom.Article;
import org.chromium.brave_news.mojom.BraveNewsController;
import org.chromium.brave_news.mojom.CardType;
import org.chromium.brave_news.mojom.Channel;
import org.chromium.brave_news.mojom.Deal;
import org.chromium.brave_news.mojom.DisplayAd;
import org.chromium.brave_news.mojom.FeedItem;
import org.chromium.brave_news.mojom.FeedItemMetadata;
import org.chromium.brave_news.mojom.LocaleInfo;
import org.chromium.brave_news.mojom.PromotedArticle;
import org.chromium.brave_news.mojom.Publisher;
import org.chromium.brave_news.mojom.PublisherType;
import org.chromium.brave_news.mojom.UserEnabled;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.brave_news.models.FeedItemCard;
import org.chromium.chrome.browser.brave_news.models.FeedItemsCard;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.settings.BraveNewsPreferencesDataListener;
import org.chromium.chrome.browser.settings.BraveNewsPreferencesV2;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;

@NullMarked
public class BraveNewsUtils {
    public static final int BRAVE_NEWS_VIEWD_CARD_TIME = 1000; // milliseconds
    private static @Nullable Map<Integer, DisplayAd> sCurrentDisplayAds;
    private static @Nullable String sLocale;
    private static @Nullable List<Channel> sChannelList;
    private static @Nullable List<Publisher> sGlobalPublisherList;
    private static @Nullable List<Publisher> sPublisherList;
    private static @Nullable List<Channel> sFollowingChannelList;
    private static @Nullable List<Publisher> sFollowingPublisherList;
    private static @Nullable List<String> sSuggestionsList;
    private static final HashMap<String, Integer> sChannelIcons = new HashMap<>();

    public static String getPromotionIdItem(FeedItemsCard items) {
        String creativeInstanceId = "null";
        if (items.getFeedItems() != null) {
            for (FeedItemCard itemCard : items.getFeedItems()) {
                FeedItem item = itemCard.getFeedItem();
                if (item.which() == FeedItem.Tag.PromotedArticle) {
                    PromotedArticle promotedArticle = item.getPromotedArticle();
                    creativeInstanceId = promotedArticle.creativeInstanceId;
                    break;
                }
            }
        }

        return creativeInstanceId;
    }

    public static void initCurrentAds() {
        sCurrentDisplayAds = new HashMap<>();
    }

    public static void putToDisplayAdsMap(Integer index, DisplayAd ad) {
        if (sCurrentDisplayAds == null) {
            sCurrentDisplayAds = new HashMap<>();
        }
        sCurrentDisplayAds.put(index, ad);
    }

    @Nullable
    public static DisplayAd getFromDisplayAdsMap(Integer index) {
        if (sCurrentDisplayAds != null) {
            DisplayAd foundItem = sCurrentDisplayAds.get(index);
            return foundItem;
        }

        return null;
    }

    // method for logging news object. works by putting Log.d in the desired places of the parsing
    // of the object
    @SuppressWarnings("UnusedVariable")
    public static void logFeedItem(FeedItemsCard items, String id) {
        if (items != null) {
            if (items.getCardType() == CardType.DISPLAY_AD) {
                DisplayAd displayAd = items.getDisplayAd();
                if (displayAd != null) {
                    Log.d("bn", id + " DISPLAY_AD title: " + displayAd.title);
                }
            } else {
                if (items.getFeedItems() != null) {
                    int index = 0;
                    for (FeedItemCard itemCard : items.getFeedItems()) {
                        if (index > 50) {
                            return;
                        }
                        FeedItem item = itemCard.getFeedItem();

                        switch (item.which()) {
                            case FeedItem.Tag.Article:
                                Article article = item.getArticle();
                                FeedItemMetadata articleData = article.data;
                                break;
                            case FeedItem.Tag.PromotedArticle:
                                PromotedArticle promotedArticle = item.getPromotedArticle();
                                FeedItemMetadata promotedArticleData = promotedArticle.data;
                                String creativeInstanceId = promotedArticle.creativeInstanceId;
                                break;
                            case FeedItem.Tag.Deal:
                                Deal deal = item.getDeal();
                                FeedItemMetadata dealData = deal.data;
                                String offersCategory = deal.offersCategory;
                                break;
                        }
                        index++;
                    }
                }
            }
        }
    }

    public static boolean shouldDisplayNewsFeed() {
        return BravePrefServiceBridge.getInstance().getShowNews()
                && BravePrefServiceBridge.getInstance().getNewsOptIn();
    }

    public static boolean shouldDisplayNewsOptin() {
        return BravePrefServiceBridge.getInstance().getShowNews()
                && !BravePrefServiceBridge.getInstance().getNewsOptIn()
                && ChromeSharedPreferences.getInstance()
                        .readBoolean(BraveNewsPreferencesV2.PREF_SHOW_OPTIN, true);
    }

    public static void setChannelIcons() {
        sChannelIcons.put("Brave", R.drawable.ic_channel_brave);
        sChannelIcons.put("Business", R.drawable.ic_channel_business);
        sChannelIcons.put("Cars", R.drawable.ic_channel_cars);
        sChannelIcons.put("Crypto", R.drawable.ic_channel_crypto);
        sChannelIcons.put("Culture", R.drawable.ic_channel_culture);
        sChannelIcons.put("Entertainment", R.drawable.ic_channel_entertainment);
        sChannelIcons.put("Entertainment News", R.drawable.ic_channel_entertainment);
        sChannelIcons.put("Fashion", R.drawable.ic_channel_fashion);
        sChannelIcons.put("Film and TV", R.drawable.ic_channel_filmtv);
        sChannelIcons.put("Food", R.drawable.ic_channel_food);
        sChannelIcons.put("Fun", R.drawable.ic_channel_fun);
        sChannelIcons.put("Gaming", R.drawable.ic_channel_gaming);
        sChannelIcons.put("Health", R.drawable.ic_channel_health);
        sChannelIcons.put("Home", R.drawable.ic_channel_home);
        sChannelIcons.put("Music", R.drawable.ic_channel_music);
        sChannelIcons.put("Politics", R.drawable.ic_channel_politics);
        sChannelIcons.put("Regional News", R.drawable.ic_channel_us_news);
        sChannelIcons.put("Science", R.drawable.ic_channel_science);
        sChannelIcons.put("Sports", R.drawable.ic_channel_sports);
        sChannelIcons.put("Technology", R.drawable.ic_channel_technology);
        sChannelIcons.put("Tech News", R.drawable.ic_channel_technology);
        sChannelIcons.put("Tech Reviews", R.drawable.ic_channel_tech_reviews);
        sChannelIcons.put("Top News", R.drawable.ic_channel_top_news);
        sChannelIcons.put("Travel", R.drawable.ic_channel_travel);
        sChannelIcons.put("UK News", R.drawable.ic_channel_us_news);
        sChannelIcons.put("US News", R.drawable.ic_channel_us_news);
        sChannelIcons.put("Weather", R.drawable.ic_channel_weather);
        sChannelIcons.put("World News", R.drawable.ic_channel_world_news);
    }

    public static HashMap<String, Integer> getChannelIcons() {
        return sChannelIcons;
    }

    private static void setLocale(String locale) {
        sLocale = locale;
    }

    public static String getLocale() {
        if (sLocale == null) {
            return "";
        }
        return sLocale;
    }

    private static void setChannelList(List<Channel> channelList) {
        sChannelList = channelList;
        setFollowingChannelList();
    }

    @Nullable
    public static List<Channel> getChannelList() {
        return sChannelList;
    }

    private static void setPopularSources(List<Publisher> publisherList) {
        sPublisherList = publisherList;
        setFollowingPublisherList();
    }

    public static List<Publisher> getPopularSources() {
        if (sPublisherList == null) {
            return new ArrayList<>();
        }

        return sPublisherList;
    }

    private static void setSuggestionsIds(List<String> suggestionsList) {
        sSuggestionsList = suggestionsList;
    }

    public static List<Publisher> getSuggestionsPublisherList() {
        List<Publisher> suggestionsPublisherList = new ArrayList<>();
        if (sPublisherList != null && sSuggestionsList != null && sSuggestionsList.size() > 0) {
            for (Publisher publisher : sPublisherList) {
                if (sSuggestionsList.contains(publisher.publisherId)) {
                    suggestionsPublisherList.add(publisher);
                }
            }
        }
        return suggestionsPublisherList;
    }

    public static void setFollowingPublisherList() {
        List<Publisher> publisherList = new ArrayList<>();
        if (sGlobalPublisherList != null && sGlobalPublisherList.size() > 0) {
            for (Publisher publisher : sGlobalPublisherList) {
                if (publisher.userEnabledStatus == UserEnabled.ENABLED
                        || (publisher.type == PublisherType.DIRECT_SOURCE
                                && publisher.userEnabledStatus != UserEnabled.DISABLED)) {
                    publisherList.add(publisher);
                }
            }
        }
        sFollowingPublisherList = publisherList;
    }

    public static void disableFollowingPublisherList(String publisherId) {
        if (sFollowingPublisherList == null) {
            return;
        }
        for (Publisher publisher : sFollowingPublisherList) {
            if (publisher.publisherId.equals(publisherId)) {
                publisher.userEnabledStatus = UserEnabled.DISABLED;
                break;
            }
        }
    }

    public static List<Publisher> getFollowingPublisherList() {
        if (sFollowingPublisherList == null) {
            return new ArrayList<>();
        }
        return sFollowingPublisherList;
    }

    public static void setFollowingChannelList() {
        List<Channel> channelList = new ArrayList<>();
        if (sLocale != null && sChannelList != null && sChannelList.size() > 0) {
            for (Channel channel : sChannelList) {
                List<String> subscribedLocalesList =
                        new ArrayList<>(Arrays.asList(channel.subscribedLocales));
                if (subscribedLocalesList.contains(sLocale)) {
                    channelList.add(channel);
                }
            }
        }
        sFollowingChannelList = channelList;
    }

    @Nullable
    public static List<Channel> getFollowingChannelList() {
        return sFollowingChannelList;
    }

    public static List<Channel> searchChannel(String search) {
        List<Channel> channelList = new ArrayList<>();
        if (sChannelList != null && sChannelList.size() > 0) {
            for (Channel channel : sChannelList) {
                if (channel.channelName.toLowerCase(Locale.ROOT).contains(search)) {
                    channelList.add(channel);
                }
            }
        }
        return channelList;
    }

    public static List<Publisher> searchPublisher(String search) {
        List<Publisher> publisherList = new ArrayList<>();
        if (sGlobalPublisherList != null && sGlobalPublisherList.size() > 0) {
            for (Publisher publisher : sGlobalPublisherList) {
                if (publisher.publisherName.toLowerCase(Locale.ROOT).contains(search)
                        || publisher.categoryName.toLowerCase(Locale.ROOT).contains(search)
                        || publisher.feedSource.url.toLowerCase(Locale.ROOT).contains(search)
                        || publisher.siteUrl.url.toLowerCase(Locale.ROOT).contains(search)) {
                    publisherList.add(publisher);
                }
            }
        }

        return publisherList;
    }

    public static boolean searchPublisherForRss(String feedUrl) {
        boolean isFound = false;
        if (sGlobalPublisherList != null && sGlobalPublisherList.size() > 0) {
            for (Publisher publisher : sGlobalPublisherList) {
                if (publisher.feedSource.url.equalsIgnoreCase(feedUrl)
                        || publisher.siteUrl.url.equalsIgnoreCase(feedUrl)) {
                    isFound = true;
                    break;
                }
            }
        }
        return isFound;
    }

    public static void getBraveNewsSettingsDataPerProfile(Profile profile) {
        ThreadUtils.assertOnUiThread();
        BraveNewsControllerFactory.getInstance()
                .getForProfile(profile, null)
                .then(
                        braveNewsController -> {
                            if (braveNewsController == null) {
                                return;
                            }
                            BraveNewsUtils.getBraveNewsSettingsData(
                                    braveNewsController,
                                    null,
                                    () -> {
                                        braveNewsController.close();
                                    });
                        });
    }

    public static void getBraveNewsSettingsData(
            BraveNewsController braveNewsController,
            @Nullable BraveNewsPreferencesDataListener braveNewsPreferencesDataListener,
            final Runnable postAction) {
        PostTask.postTask(
                TaskTraits.BEST_EFFORT,
                () -> {
                    if (braveNewsController == null) {
                        return;
                    }
                    braveNewsController.getLocale(
                            (locale) -> {
                                setLocale(locale);
                                getChannels(
                                        braveNewsController,
                                        braveNewsPreferencesDataListener,
                                        () -> {
                                            getPublishers(
                                                    braveNewsController,
                                                    braveNewsPreferencesDataListener,
                                                    () -> {
                                                        getSuggestionsSources(
                                                                braveNewsController,
                                                                braveNewsPreferencesDataListener,
                                                                postAction);
                                                    });
                                        });
                            });
                });
    }

    private static void getChannels(
            BraveNewsController braveNewsController,
            @Nullable BraveNewsPreferencesDataListener braveNewsPreferencesDataListener,
            final Runnable postAction) {
        braveNewsController.getChannels(
                (channels) -> {
                    List<Channel> channelList = new ArrayList<>();
                    for (Map.Entry<String, Channel> entry : channels.entrySet()) {
                        Channel channel = entry.getValue();
                        channelList.add(channel);
                    }

                    Comparator<Channel> compareByName =
                            (Channel o1, Channel o2) ->
                                    o1.channelName
                                            .toLowerCase(Locale.ROOT)
                                            .compareTo(o2.channelName.toLowerCase(Locale.ROOT));
                    Collections.sort(channelList, compareByName);

                    setChannelList(channelList);
                    if (braveNewsPreferencesDataListener != null) {
                        braveNewsPreferencesDataListener.onChannelReceived();
                    }
                    if (postAction != null) {
                        postAction.run();
                    }
                });
    }

    private static void getPublishers(
            BraveNewsController braveNewsController,
            @Nullable BraveNewsPreferencesDataListener braveNewsPreferencesDataListener,
            final Runnable postAction) {
        braveNewsController.getPublishers(
                (publishers) -> {
                    setPublishers(publishers);
                    if (braveNewsPreferencesDataListener != null) {
                        braveNewsPreferencesDataListener.onPublisherReceived();
                    }
                    if (postAction != null) {
                        postAction.run();
                    }
                });
    }

    public static void setPublishers(Map<String, Publisher> publishers) {
        List<Publisher> globalPublisherList = new ArrayList<>();
        List<Publisher> publisherList = new ArrayList<>();
        HashMap<String, LocaleInfo> localesMap = new HashMap<>();
        for (Map.Entry<String, Publisher> entry : publishers.entrySet()) {
            Publisher publisher = entry.getValue();
            globalPublisherList.add(publisher);
            if (sLocale == null) {
                continue;
            }
            for (LocaleInfo localeInfo : publisher.locales) {
                if (localeInfo.locale.equals(sLocale)) {
                    localesMap.put(publisher.publisherId, localeInfo);
                    publisherList.add(publisher);
                    break;
                }
            }
        }

        Comparator<Publisher> compareByName = (Publisher o1, Publisher o2)
                -> o1.publisherName.toLowerCase(Locale.ROOT)
                           .compareTo(o2.publisherName.toLowerCase(Locale.ROOT));

        Collections.sort(globalPublisherList, compareByName);

        sGlobalPublisherList = globalPublisherList;

        Collections.sort(publisherList, compareByName);

        Comparator<Publisher> compareByRank =
                (Publisher o1, Publisher o2) ->
                        Integer.compare(
                                localesMap.get(o1.publisherId) == null
                                        ? -1
                                        : localesMap.get(o1.publisherId).rank,
                                localesMap.get(o2.publisherId) == null
                                        ? -1
                                        : localesMap.get(o2.publisherId).rank);

        Collections.sort(publisherList, compareByRank);

        setPopularSources(publisherList);
    }

    public static void getSuggestionsSources(
            BraveNewsController braveNewsController,
            @Nullable BraveNewsPreferencesDataListener braveNewsPreferencesDataListener,
            final Runnable postAction) {
        braveNewsController.getSuggestedPublisherIds(
                (publisherIds) -> {
                    setSuggestionsIds(Arrays.asList(publisherIds));
                    if (braveNewsPreferencesDataListener != null) {
                        braveNewsPreferencesDataListener.onSuggestionsReceived();
                    }
                    if (postAction != null) {
                        postAction.run();
                    }
                });
    }
}
