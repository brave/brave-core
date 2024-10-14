/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.brave_news.models;

import org.chromium.brave_news.mojom.DisplayAd;

import java.util.Arrays;
import java.util.List;

public class FeedItemsCard {
    private List<FeedItemCard> feedItems;
    private int cardType;
    private byte[] imageByte;
    private String uuid;
    private boolean viewStatSent;
    private DisplayAd displayAd;

    public FeedItemsCard() {
        this.viewStatSent = false;
    }

    public List<FeedItemCard> getFeedItems() {
        return feedItems;
    }

    public void setFeedItems(List<FeedItemCard> feedItems) {
        this.feedItems = feedItems;
    }

    public int getCardType() {
        return cardType;
    }

    public void setCardType(int cardType) {
        this.cardType = cardType;
    }

    public byte[] getImageByte() {
        return imageByte;
    }

    public void setImageByte(byte[] imageByte) {
        this.imageByte = imageByte;
    }

    public String getUuid() {
        return uuid;
    }

    public void setUuid(String uuid) {
        this.uuid = uuid;
    }

    public boolean isViewStatSent() {
        return viewStatSent;
    }

    public void setViewStatSent(boolean viewStatSent) {
        this.viewStatSent = viewStatSent;
    }

    public DisplayAd getDisplayAd() {
        return displayAd;
    }

    public void setDisplayAd(DisplayAd displayAd) {
        this.displayAd = displayAd;
    }

    @SuppressWarnings("ObjectToString")
    @Override
    public String toString() {
        return "FeedItemsCard{"
                + "feedItems=" + feedItems + ", cardType=" + cardType + ", displayAd=" + displayAd
                + ", imageByte=" + Arrays.toString(imageByte) + '}';
    }
}
