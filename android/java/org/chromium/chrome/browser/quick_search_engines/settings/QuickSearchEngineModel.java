/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.quick_search_engines.settings;

import org.chromium.components.search_engines.TemplateUrl;

public class QuickSearchEngineModel {
    private TemplateUrl mTemplateUrl;
    private boolean mIsEnabled;
    private int mPosition;

    QuickSearchEngineModel(TemplateUrl templateUrl, boolean isEnabled, int position) {
        this.mTemplateUrl = templateUrl;
        this.mIsEnabled = isEnabled;
        this.mPosition = position;
    }

    public TemplateUrl getTemplateUrl() {
        return mTemplateUrl;
    }

    public boolean isEnabled() {
        return mIsEnabled;
    }

    public int getPosition() {
        return mPosition;
    }
}
