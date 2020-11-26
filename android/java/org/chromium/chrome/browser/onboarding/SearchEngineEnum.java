/**
 * Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.onboarding;

import org.chromium.chrome.R;

public enum SearchEngineEnum {
    GOOGLE(R.drawable.search_engine_google, SearchEngineEnumConstants.SEARCH_GOOGLE_ID),
    YAHOO(R.drawable.search_engine_yahoo, SearchEngineEnumConstants.SEARCH_YAHOO_ID),
    DUCKDUCKGO(R.drawable.search_engine_duckduckgo, SearchEngineEnumConstants.SEARCH_DUCKDUCKGO_ID),
    DUCKDUCKGOLITE(R.drawable.search_engine_duckduckgo_lite,
            SearchEngineEnumConstants.SEARCH_DUCKDUCKGO_LITE_ID),
    QWANT(R.drawable.search_engine_qwant, SearchEngineEnumConstants.SEARCH_QWANT_ID),
    BING(R.drawable.search_engine_bing, SearchEngineEnumConstants.SEARCH_BING_ID),
    YANDEX(R.drawable.yandex, SearchEngineEnumConstants.SEARCH_YANDEX_ID),
    STARTPAGE(R.drawable.search_engine_startpage, SearchEngineEnumConstants.SEARCH_STARTPAGE_ID);

    private int icon;
    private int id;

    SearchEngineEnum(int icon, int id) {
        this.icon = icon;
        this.id = id;
    }

    public int getIcon() {
        return icon;
    }

    public int getId() {
        return id;
    }

    interface SearchEngineEnumConstants {
        static final int SEARCH_GOOGLE_ID = 0;
        static final int SEARCH_DUCKDUCKGO_ID = 1;
        static final int SEARCH_DUCKDUCKGO_LITE_ID = 2;
        static final int SEARCH_QWANT_ID = 3;
        static final int SEARCH_BING_ID = 4;
        static final int SEARCH_STARTPAGE_ID = 5;
        static final int SEARCH_YAHOO_ID = 6;
        static final int SEARCH_YANDEX_ID = 7;
    }
}
