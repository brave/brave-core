/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.omnibox.suggestions;

import android.content.Context;

import androidx.annotation.NonNull;

import org.chromium.base.BraveReflectionUtil;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.omnibox.LocationBarDataProvider;
import org.chromium.components.omnibox.AutocompleteMatch;
import org.chromium.components.omnibox.OmniboxSuggestionType;
import org.chromium.misc_metrics.mojom.MiscAndroidMetrics;
import org.chromium.url.GURL;

class BraveAutocompleteMediatorBase {
    void loadUrlForOmniboxMatch(
            int matchIndex,
            @NonNull AutocompleteMatch suggestion,
            @NonNull GURL url,
            long inputStart,
            boolean openInNewTab,
            boolean openInNewWindow) {
        Context context =
                (Context)
                        BraveReflectionUtil.getField(AutocompleteMediator.class, "mContext", this);
        LocationBarDataProvider dataProvider =
                (LocationBarDataProvider)
                        BraveReflectionUtil.getField(
                                AutocompleteMediator.class, "mDataProvider", this);

        if (dataProvider != null && context != null && context instanceof BraveActivity) {
            MiscAndroidMetrics miscAndroidMetrics =
                    ((BraveActivity) context).getMiscAndroidMetrics();
            if (miscAndroidMetrics != null) {
                int type = suggestion.getType();
                boolean isSearchQuery =
                        type == OmniboxSuggestionType.SEARCH_WHAT_YOU_TYPED
                                || type == OmniboxSuggestionType.SEARCH_HISTORY
                                || type == OmniboxSuggestionType.SEARCH_SUGGEST
                                || type == OmniboxSuggestionType.SEARCH_SUGGEST_ENTITY
                                || type == OmniboxSuggestionType.SEARCH_SUGGEST_TAIL
                                || type == OmniboxSuggestionType.SEARCH_SUGGEST_PERSONALIZED
                                || type == OmniboxSuggestionType.SEARCH_SUGGEST_PROFILE
                                || type == OmniboxSuggestionType.SEARCH_OTHER_ENGINE;

                if (!dataProvider.isIncognito()) {
                    boolean isNewTab = dataProvider.getNewTabPageDelegate().isCurrentlyVisible();
                    miscAndroidMetrics.recordLocationBarChange(isNewTab, isSearchQuery);
                }

                if (isSearchQuery) {
                    boolean isSuggestion = type != OmniboxSuggestionType.SEARCH_WHAT_YOU_TYPED;
                    miscAndroidMetrics.recordOmniboxSearchQuery(url.getSpec(), isSuggestion);
                }

                if (url.getScheme().startsWith("http")) {
                    if (type == OmniboxSuggestionType.URL_WHAT_YOU_TYPED) {
                        miscAndroidMetrics.recordOmniboxDirectNavigation();
                    } else if (type == OmniboxSuggestionType.HISTORY_URL
                            || type == OmniboxSuggestionType.HISTORY_TITLE
                            || type == OmniboxSuggestionType.HISTORY_BODY
                            || type == OmniboxSuggestionType.HISTORY_KEYWORD) {
                        miscAndroidMetrics.recordOmniboxHistoryNavigation();
                    } else if (type == OmniboxSuggestionType.BOOKMARK_TITLE) {
                        miscAndroidMetrics.recordOmniboxBookmarkNavigation();
                    }
                }
            }
        }

        BraveReflectionUtil.invokeMethod(
                AutocompleteMediator.class,
                this,
                "loadUrlForOmniboxMatch",
                int.class,
                matchIndex,
                AutocompleteMatch.class,
                suggestion,
                GURL.class,
                url,
                long.class,
                inputStart,
                boolean.class,
                openInNewTab,
                boolean.class,
                openInNewWindow);
    }
}
