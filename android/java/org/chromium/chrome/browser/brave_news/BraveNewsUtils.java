/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.brave_news;

import android.content.Context;

import org.chromium.base.Log;
import org.chromium.brave_news.mojom.Article;
import org.chromium.brave_news.mojom.CardType;
import org.chromium.brave_news.mojom.Deal;
import org.chromium.brave_news.mojom.DisplayAd;
import org.chromium.brave_news.mojom.FeedItem;
import org.chromium.brave_news.mojom.FeedItemMetadata;
import org.chromium.brave_news.mojom.PromotedArticle;
import org.chromium.brave_news.mojom.Publisher;
import org.chromium.chrome.browser.brave_news.models.FeedItemCard;
import org.chromium.chrome.browser.brave_news.models.FeedItemsCard;

import java.util.HashMap;
import java.util.Map;

public class BraveNewsUtils {
    public static final int BRAVE_NEWS_VIEWD_CARD_TIME = 100; // milliseconds
    private static Map<Integer, DisplayAd> sCurrentDisplayAds;

    public static String getPromotionIdItem(FeedItemsCard items) {
        String creativeInstanceId = "null";
        if (items.getFeedItems() != null) {
            for (FeedItemCard itemCard : items.getFeedItems()) {
                FeedItem item = itemCard.getFeedItem();
                FeedItemMetadata itemMetaData = new FeedItemMetadata();
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

    public static DisplayAd getFromDisplayAdsMap(Integer index) {
        if (sCurrentDisplayAds != null) {
            DisplayAd foundItem = sCurrentDisplayAds.get(index);
            return foundItem;
        }

        return null;
    }

    // method for logging news object. works by putting Log.d in the desired places of the parsing
    // of the object
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
}
