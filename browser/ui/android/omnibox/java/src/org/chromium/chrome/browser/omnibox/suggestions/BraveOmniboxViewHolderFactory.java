/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.omnibox.suggestions;

import android.view.LayoutInflater;
import android.view.View;

import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.omnibox.brave.R;
import org.chromium.chrome.browser.omnibox.suggestions.base.BaseSuggestionView;
import org.chromium.chrome.browser.omnibox.suggestions.base.BaseSuggestionViewBinder;
import org.chromium.chrome.browser.omnibox.suggestions.brave_leo.BraveLeoSuggestionViewBinder;
import org.chromium.chrome.browser.omnibox.suggestions.brave_search.BraveSearchBannerViewBinder;

/** Brave extension of {@link OmniboxViewHolderFactory} that registers Brave suggestion types. */
@NullMarked
public class BraveOmniboxViewHolderFactory extends BraveOmniboxViewHolderFactoryDummySuper {
    @SuppressWarnings("unchecked")
    public BraveOmniboxViewHolderFactory() {
        super();

        registerType(
                BraveOmniboxSuggestionUiType.BRAVE_SEARCH_PROMO_BANNER,
                parent ->
                        LayoutInflater.from(parent.getContext())
                                .inflate(R.layout.omnibox_brave_search_banner, null),
                BraveSearchBannerViewBinder::bind);

        registerType(
                BraveOmniboxSuggestionUiType.BRAVE_LEO_SUGGESTION,
                parent ->
                        new BaseSuggestionView<View>(
                                parent.getContext(), R.layout.omnibox_basic_suggestion),
                new BaseSuggestionViewBinder<View>(BraveLeoSuggestionViewBinder::bind));
    }
}
