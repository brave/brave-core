/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.brave_news.models;

import org.chromium.brave_news.mojom.FeedItem;

import java.util.Arrays;

public class FeedItemCard {
    private FeedItem feedItem;
    private int cardType;
    private byte[] imageByte;

    public FeedItemCard() {}

    public FeedItem getFeedItem() {
        return feedItem;
    }

    public void setFeedItem(FeedItem feedItem) {
        this.feedItem = feedItem;
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

    @SuppressWarnings("ObjectToString")
    @Override
    public String toString() {
        return "FeedItemCard{"
                + "feedItem=" + feedItem + ", cardType=" + cardType + ", imageByte='"
                + Arrays.toString(imageByte) + '\'' + '}';
    }
}
