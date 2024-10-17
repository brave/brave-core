/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.quick_search_engines.settings;

public class QuickSearchEngineModel {
    // private TemplateUrl mTemplateUrl;
    private String mShortName;
    private String mKeyword;
    private String mUrl;
    private boolean mIsEnabled;
    private int mPosition;

    // QuickSearchEngineModel(TemplateUrl templateUrl, boolean isEnabled, int position) {
    //     this.mTemplateUrl = templateUrl;
    //     this.mIsEnabled = isEnabled;
    //     this.mPosition = position;
    // }

    // public TemplateUrl getTemplateUrl() {
    //     return mTemplateUrl;
    // }

    public QuickSearchEngineModel(
            String shortName, String keyword, String url, boolean isEnabled, int position) {
        this.mShortName = shortName;
        this.mKeyword = keyword;
        this.mUrl = url;
        this.mIsEnabled = isEnabled;
        this.mPosition = position;
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

    public int getPosition() {
        return mPosition;
    }

    public void setEnabled(boolean isEnabled) {
        this.mIsEnabled = isEnabled;
    }
}
