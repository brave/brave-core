/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.local_database;

public class BraveStatsTable {
    public static final String TABLE_NAME = "brave_stats_table";

    public static final String COLUMN_URL = "url";
    public static final String COLUMN_DOMAIN = "domain";
    public static final String COLUMN_STAT_TYPE = "stat_type";
    public static final String COLUMN_STAT_SITE = "stat_site"; 
    public static final String COLUMN_STAT_SITE_DOMAIN = "stat_site_domain"; 
    public static final String COLUMN_TIMESTAMP = "timestamp";
    // Create table SQL query
    public static final String CREATE_TABLE =
        "CREATE TABLE IF NOT EXISTS " + TABLE_NAME + "( ID INTEGER PRIMARY KEY AUTOINCREMENT,"
        + COLUMN_URL + " TEXT,"
        + COLUMN_DOMAIN + " TEXT,"
        + COLUMN_STAT_TYPE + " TEXT,"
        + COLUMN_STAT_SITE + " TEXT,"
        + COLUMN_STAT_SITE_DOMAIN + " TEXT,"
        + COLUMN_TIMESTAMP + " DATETIME"
        + ")";

    public BraveStatsTable() {
    }

    private String mUrl;
    private String mDomain;
    private String mStatType;
    private String mStatSite;
    private String mStatSiteDomain;
    private String mTimestamp;

    public BraveStatsTable(String url, String domain, String statType, String statSite, String statSiteDomain, String timestamp) {
        mUrl = url;
        mDomain = domain;
        mStatType = statType;
        mStatSite = statSite;
        mStatSiteDomain = statSiteDomain;
        mTimestamp = timestamp;
    }

    public String getUrl() {
        return mUrl;
    }

    public String getDomain() {
        return mDomain;
    }

    public String getStatType() {
        return mStatType;
    }

    public String getStatSite() {
        return mStatSite;
    }

    public String getStatSiteDomain() {
        return mStatSiteDomain;
    }

    public String getTimestamp() {
        return mTimestamp;
    }
}