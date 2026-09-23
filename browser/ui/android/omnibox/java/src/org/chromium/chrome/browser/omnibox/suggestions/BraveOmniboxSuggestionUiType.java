/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.omnibox.suggestions;

import org.chromium.components.omnibox.suggestions.OmniboxSuggestionUiType;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

@Retention(RetentionPolicy.SOURCE)
public @interface BraveOmniboxSuggestionUiType {
    int BRAVE_SEARCH_PROMO_BANNER = OmniboxSuggestionUiType.COUNT + 1;
    int BRAVE_LEO_SUGGESTION = BRAVE_SEARCH_PROMO_BANNER + 1;
}
