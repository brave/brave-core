/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.local_database;

public class TopSiteTable {
    public static final String TABLE_NAME = "top_site_table";

    public static final String COLUMN_NAME = "name";
    public static final String COLUMN_DESTINATION_URL = "destination_url";
    public static final String COLUMN_BACKGROUND_COLOR = "background_color";
    public static final String COLUMN_IMAGE_PATH = "image_path";

    // Create table SQL query
    public static final String CREATE_TABLE =
            "CREATE TABLE IF NOT EXISTS " + TABLE_NAME + "("
                    + COLUMN_NAME + " TEXT,"
                    + COLUMN_DESTINATION_URL + " TEXT,"
                    + COLUMN_BACKGROUND_COLOR + " TEXT,"
                    + COLUMN_IMAGE_PATH + " TEXT"
                    + ")";

    public TopSiteTable() {
    }

    private String mName;
    private String mDestinationUrl;
    private String mBackgroundColor;
    private String mImagePath;

    public TopSiteTable(String name, String destinationUrl, String backgroundColor, String imagePath) {
        mName = name;
        mDestinationUrl = destinationUrl;
        mBackgroundColor = backgroundColor;
        mImagePath = imagePath;
    }

    public String getName() {
        return mName;
    }        

    public String getDestinationUrl() {
        return mDestinationUrl;
    }

    public String getBackgroundColor() {
        return mBackgroundColor;
    }

    public String getImagePath() {
        return mImagePath;
    }
}