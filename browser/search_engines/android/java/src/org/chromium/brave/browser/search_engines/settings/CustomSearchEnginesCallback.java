/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.search_engines.settings;

import android.widget.ImageView;

public interface CustomSearchEnginesCallback {
    public default void onSearchEngineClick(String searchEngineKeyword) {}

    public default void loadSearchEngineLogo(ImageView logoView, String searchEngineKeyword) {}

    public default void removeSearchEngine(String searchEngineKeyword) {}
}
