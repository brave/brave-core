/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.search_engines.settings;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.components.search_engines.TemplateUrl;

/**
 * Dummy super class that allows BraveBaseSearchEngineAdapter to call the private
 * SearchEngineAdapter.getSearchEngineSourceType via bytecode modification.
 */
@NullMarked
public class BraveSearchEngineAdapterDummySuper {
    public static @SearchEngineAdapter.TemplateUrlSourceType int getSearchEngineSourceType(
            TemplateUrl templateUrl, @Nullable TemplateUrl defaultSearchEngine) {
        assert false : "This class usage should be removed via bytecode modification!";
        return SearchEngineAdapter.TemplateUrlSourceType.PREPOPULATED;
    }
}
