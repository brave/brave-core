/**
 * Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.onboarding;

import org.chromium.chrome.R;

public enum SearchEngineEnum {
    GOOGLE(R.drawable.search_engine_google, SearchEngineEnumConstants.SEARCH_GOOGLE_ID,
            R.string.google_desc),
    BRAVE(R.drawable.search_engine_brave, SearchEngineEnumConstants.SEARCH_BRAVE_ID,
            R.string.brave_desc),
    DUCKDUCKGO(R.drawable.search_engine_duckduckgo, SearchEngineEnumConstants.SEARCH_DUCKDUCKGO_ID,
            R.string.ddg_desc),
    QWANT(R.drawable.search_engine_qwant, SearchEngineEnumConstants.SEARCH_QWANT_ID,
            R.string.qwant_desc),
    BING(R.drawable.search_engine_bing, SearchEngineEnumConstants.SEARCH_BING_ID,
            R.string.bing_desc),
    YANDEX(R.drawable.yandex, SearchEngineEnumConstants.SEARCH_YANDEX_ID, R.string.yandex_desc),
    STARTPAGE(R.drawable.search_engine_startpage, SearchEngineEnumConstants.SEARCH_STARTPAGE_ID,
            R.string.startpage_desc),
    ECOSIA(R.drawable.ecosia, SearchEngineEnumConstants.SEARCH_ECOSIA_ID, R.string.ecosia_desc),
    DAUM(R.drawable.daum, SearchEngineEnumConstants.SEARCH_DAUM_ID, R.string.daum_desc),
    NAVER(R.drawable.naver, SearchEngineEnumConstants.SEARCH_NAVER_ID, R.string.naver_desc);

    private final int icon;
    private final int id;
    private final int desc;

    SearchEngineEnum(int icon, int id, int desc) {
        this.icon = icon;
        this.id = id;
        this.desc = desc;
    }

    public int getIcon() {
        return icon;
    }

    public int getId() {
        return id;
    }

    public int getDesc() {
        return desc;
    }

    interface SearchEngineEnumConstants {
        static final int SEARCH_GOOGLE_ID = 0;
        static final int SEARCH_BRAVE_ID = 1;
        static final int SEARCH_DUCKDUCKGO_ID = 2;
        static final int SEARCH_QWANT_ID = 3;
        static final int SEARCH_BING_ID = 4;
        static final int SEARCH_STARTPAGE_ID = 5;
        static final int SEARCH_YANDEX_ID = 6;
        static final int SEARCH_ECOSIA_ID = 7;
        static final int SEARCH_DAUM_ID = 8;
        static final int SEARCH_NAVER_ID = 9;
    }
}
