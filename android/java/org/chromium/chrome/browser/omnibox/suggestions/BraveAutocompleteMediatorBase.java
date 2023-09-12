/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.omnibox.suggestions;

import android.content.Context;

import androidx.annotation.NonNull;

import org.chromium.base.BraveReflectionUtil;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.components.omnibox.AutocompleteMatch;
import org.chromium.components.omnibox.OmniboxSuggestionType;
import org.chromium.url.GURL;

class BraveAutocompleteMediatorBase {
    void loadUrlForOmniboxMatch(int matchIndex, @NonNull AutocompleteMatch suggestion,
            @NonNull GURL url, long inputStart, boolean inVisibleSuggestionList) {
        BraveReflectionUtil.InvokeMethod(AutocompleteMediator.class, this, "loadUrlForOmniboxMatch",
                int.class, matchIndex, AutocompleteMatch.class, suggestion, GURL.class, url,
                long.class, inputStart, boolean.class, inVisibleSuggestionList);
        if (suggestion.getType() == OmniboxSuggestionType.SEARCH_WHAT_YOU_TYPED
                || suggestion.getType() == OmniboxSuggestionType.SEARCH_SUGGEST) {
            Context context = (Context) BraveReflectionUtil.getField(
                    AutocompleteMediator.class, "mContext", this);
            if (context != null && context instanceof BraveActivity) {
                ((BraveActivity) context).getMiscAndroidMetrics().recordLocationBarQuery();
            }
        }
    }
}
