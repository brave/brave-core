/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.quick_search_engines.settings;

import android.widget.ImageView;

import java.util.List;

public interface QuickSearchEnginesCallback {
    default void onSearchEngineClick(
            int position, QuickSearchEnginesModel quickSearchEnginesModel) {}

    default void onSearchEnginesReordered(
            List<QuickSearchEnginesModel> quickSearchEngines) {}

    default void loadSearchEngineLogo(
            ImageView logoView, QuickSearchEnginesModel quickSearchEnginesModel) {}
}
