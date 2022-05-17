/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.local_database;

public class DisplayAdsTable {
    public static final String TABLE_NAME = "display_ads_table";

    public static final String COLUMN_UUID = "uuid";
    public static final String COLUMN_CREATIVE_INSTANCE_ID = "creative_instance_id";
    public static final String COLUMN_POSITION = "position";
    public static final String COLUMN_TAB_ID = "tab_id";
    public static final String COLUMN_AD_TITLE = "ad_title";
    public static final String COLUMN_AD_DESCRIPTION = "ad_description";
    public static final String COLUMN_AD_CTA_TEXT = "cta_text";
    public static final String COLUMN_AD_CTA_LINK = "cta_link";
    public static final String COLUMN_AD_IMAGE = "ad_image";

    // Create table SQL query
    public static final String CREATE_TABLE = "CREATE TABLE IF NOT EXISTS " + TABLE_NAME
            + "( ID INTEGER PRIMARY KEY AUTOINCREMENT," + COLUMN_UUID + " TEXT,"
            + COLUMN_CREATIVE_INSTANCE_ID + " TEXT," + COLUMN_POSITION + " INT," + COLUMN_TAB_ID
            + " INT," + COLUMN_AD_TITLE + " TEXT," + COLUMN_AD_DESCRIPTION + " TEXT,"
            + COLUMN_AD_CTA_TEXT + " TEXT," + COLUMN_AD_CTA_LINK + " TEXT," + COLUMN_AD_IMAGE
            + " TEXT"
            + ")";

    public DisplayAdsTable() {}

    private String mUuid;
    private String mCreativeInstanceId;
    private int mPosition;
    private int mTabId;
    private String mAdTitle;
    private String mAdDescription;
    private String mAdCtaText;
    private String mAdCtaLink;
    private String mAdImage;

    public DisplayAdsTable(String creativeInstanceId, int position, int tabId, String adTitle,
            String adCtaText, String adCtaLink, String adImage) {
        mCreativeInstanceId = creativeInstanceId;
        mPosition = position;
        mTabId = tabId;
        mAdTitle = adTitle;
        mAdCtaText = adCtaText;
        mAdCtaLink = adCtaLink;
        mAdImage = adImage;
    }

    public DisplayAdsTable(String uuid, String creativeInstanceId, int position, int tabId,
            String adTitle, String adDescription, String adCtaText, String adCtaLink,
            String adImage) {
        mUuid = uuid;
        mCreativeInstanceId = creativeInstanceId;
        mPosition = position;
        mTabId = tabId;
        mAdTitle = adTitle;
        mAdDescription = adDescription;
        mAdCtaText = adCtaText;
        mAdCtaLink = adCtaLink;
        mAdImage = adImage;
    }

    public String getUuid() {
        return mUuid;
    }

    public void setUuid(String uuid) {
        mUuid = uuid;
    }

    public String getCreativeInstanceId() {
        return mCreativeInstanceId;
    }

    public void setCreativeInstanceId(String creativeInstanceId) {
        mCreativeInstanceId = creativeInstanceId;
    }

    public int getPosition() {
        return mPosition;
    }

    public void setPosition(int position) {
        mPosition = position;
    }

    public int getTabId() {
        return mTabId;
    }

    public void setTabId(int tabId) {
        mTabId = tabId;
    }

    public String getAdTitle() {
        return mAdTitle;
    }

    public void setAdTitle(String adTitle) {
        mAdTitle = adTitle;
    }

    public String getAdDescription() {
        return mAdDescription;
    }

    public void setAdDescription(String adDescription) {
        mAdDescription = adDescription;
    }

    public String getAdCtaText() {
        return mAdCtaText;
    }

    public void setAdCtaText(String adCtaText) {
        mAdCtaText = adCtaText;
    }

    public String getAdCtaLink() {
        return mAdCtaLink;
    }

    public void setAdCtaLink(String adCtaLink) {
        mAdCtaLink = adCtaLink;
    }

    public String getAdImage() {
        return mAdImage;
    }

    public void setAdImage(String adImage) {
        mAdImage = adImage;
    }
}
