/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.quick_search_engines.settings;

import androidx.annotation.IntDef;

public class QuickSearchEnginesModel {
    @IntDef({
        QuickSearchEnginesModelType.SEARCH_ENGINE,
        QuickSearchEnginesModelType.AI_ASSISTANT,
    })
    public @interface QuickSearchEnginesModelType {
        int SEARCH_ENGINE = 0;
        int AI_ASSISTANT = 1;
    }

    private final String mShortName;
    private final String mKeyword;
    private final String mUrl;
    private boolean mIsEnabled;
    private final @QuickSearchEnginesModelType int mType;

    public QuickSearchEnginesModel(
            String shortName,
            String keyword,
            String url,
            boolean isEnabled,
            @QuickSearchEnginesModelType int type) {
        mShortName = shortName;
        mKeyword = keyword;
        mUrl = url;
        mIsEnabled = isEnabled;
        mType = type;
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

    public @QuickSearchEnginesModelType int getType() {
        return mType;
    }
}
