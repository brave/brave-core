/**
 * Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.omnibox.suggestions;

import androidx.annotation.IntDef;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

@IntDef({BraveOmniboxSuggestionUiType.BRAVE_SEARCH_PROMO_BANNER})
@Retention(RetentionPolicy.SOURCE)
public @interface BraveOmniboxSuggestionUiType {
    int BRAVE_SEARCH_PROMO_BANNER = 10;
}
