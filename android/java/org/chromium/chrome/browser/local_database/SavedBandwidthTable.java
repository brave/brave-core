/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.local_database;

public class SavedBandwidthTable {
    public static final String TABLE_NAME = "saved_bandwidth_table";

    public static final String COLUMN_SAVED_BANDWIDTH = "saved_bandwidth";
    public static final String COLUMN_TIMESTAMP = "timestamp";
    // Create table SQL query
    public static final String CREATE_TABLE =
        "CREATE TABLE IF NOT EXISTS " + TABLE_NAME + "( ID INTEGER PRIMARY KEY AUTOINCREMENT,"
        + COLUMN_SAVED_BANDWIDTH + " INTEGER,"
        + COLUMN_TIMESTAMP + " DATETIME"
        + ")";

    public SavedBandwidthTable() {
    }

    private long mSavedBandwidth;
    private String mTimestamp;

    public SavedBandwidthTable(long savedBandwidth, String timestamp) {
        mSavedBandwidth = savedBandwidth;
        mTimestamp = timestamp;
    }

    public long getSavedBandwidth() {
        return mSavedBandwidth;
    }

    public String getTimestamp() {
        return mTimestamp;
    }
}