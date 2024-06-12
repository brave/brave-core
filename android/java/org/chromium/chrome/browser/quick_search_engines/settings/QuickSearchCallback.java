/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.quick_search_engines.settings;

import android.widget.ImageView;

import org.chromium.components.search_engines.TemplateUrl;

public interface QuickSearchCallback {
    public void onSearchEngineClick(TemplateUrl templateUrl);

    public void onSearchEngineLongClick();

    public void loadSearchEngineLogo(ImageView logoView, TemplateUrl templateUrl);
}
