/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.local_database;

public class BraveStatsTable {
    public static final String TABLE_NAME = "brave_stats_table";

    public static final String COLUMN_URL = "url";
    public static final String COLUMN_DOMAIN = "domain";
    public static final String COLUMN_TIMESTAMP = "timestamp";
    public static final String COLUMN_ADS_BLOCKED_TRACKERS_BLOCKED = "ads_blocked_trackers_blocked";
    public static final String COLUMN_DATA_SAVED = "data_saved";
    public static final String COLUMN_TIME_SAVED = "time_saved";

    // Create table SQL query
    public static final String CREATE_TABLE =
        "CREATE TABLE IF NOT EXISTS " + TABLE_NAME + "("
        + COLUMN_URL + " TEXT,"
        + COLUMN_DOMAIN + " TEXT,"
        + COLUMN_TIMESTAMP + " DATETIME,"
        + COLUMN_ADS_BLOCKED_TRACKERS_BLOCKED + " INTEGER,"
        + COLUMN_DATA_SAVED + " INTEGER,"
        + COLUMN_TIME_SAVED + " DOUBLE"
        + ")";

    public BraveStatsTable() {
    }

    private String mUrl;
    private String mDomain;
    private String mTimestamp;
    private int mAdsBlockedTrackersBlocked;
    private int mDataSaved;
    private double mTimeSaved;

    public BraveStatsTable(String url, String domain, String timestamp, int adsBlockedTrackerBlocked, int dataSaved, double timeSaved) {
        mUrl = url;
        mDomain = domain;
        mTimestamp = timestamp;
        mAdsBlockedTrackersBlocked = adsBlockedTrackerBlocked;
        mDataSaved = dataSaved;
        mTimeSaved = timeSaved;
    }

    public String getUrl() {
        return mUrl;
    }

    public String getDomain() {
        return mDomain;
    }

    public String getTimestamp() {
        return mTimestamp;
    }

    public int getAdsBlockedTrackersBlocked() {
        return mAdsBlockedTrackersBlocked;
    }

    public int getDataSaved() {
        return mDataSaved;
    }

    public double getTimeSaved() {
        return mTimeSaved;
    }
}