/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.quick_search_engines.settings;

public class QuickSearchEnginesModel {
    private String mShortName;
    private String mKeyword;
    private String mUrl;
    private boolean mIsEnabled;

    public QuickSearchEnginesModel(
            String shortName, String keyword, String url, boolean isEnabled) {
        mShortName = shortName;
        mKeyword = keyword;
        mUrl = url;
        mIsEnabled = isEnabled;
    }

    public String getShortName() {
        return mShortName;
    }

    public String getKeyword() {
        return mKeyword;
    }

    public String getUrl() {
        return mUrl;
    }

    public boolean isEnabled() {
        return mIsEnabled;
    }

    public void setEnabled(boolean isEnabled) {
        mIsEnabled = isEnabled;
    }
}
