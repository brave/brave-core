/**
 * Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.omnibox.suggestions;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;

import org.chromium.base.BraveReflectionUtil;
import org.chromium.base.Callback;
import org.chromium.chrome.browser.omnibox.R;
import org.chromium.chrome.browser.omnibox.suggestions.SuggestionListViewBinder.SuggestionListViewHolder;
import org.chromium.chrome.browser.omnibox.suggestions.base.BaseSuggestionView;
import org.chromium.chrome.browser.omnibox.suggestions.base.BaseSuggestionViewBinder;
import org.chromium.chrome.browser.omnibox.suggestions.brave_leo.BraveLeoSuggestionViewBinder;
import org.chromium.chrome.browser.omnibox.suggestions.brave_search.BraveSearchBannerViewBinder;
import org.chromium.ui.ViewProvider;

import java.util.ArrayList;
import java.util.List;

public class BraveAutocompleteCoordinator {
    public ViewProvider<SuggestionListViewHolder> createViewProvider(
            Context context, boolean forcePhoneStyleOmnibox) {
        ViewProvider<SuggestionListViewHolder> provider =
                (ViewProvider<SuggestionListViewHolder>)
                        BraveReflectionUtil.invokeMethod(
                                AutocompleteCoordinator.class,
                                this,
                                "createViewProvider",
                                Context.class,
                                context,
                                boolean.class,
                                forcePhoneStyleOmnibox);

        return new ViewProvider<SuggestionListViewHolder>() {
            private List<Callback<SuggestionListViewHolder>> mCallbacks = new ArrayList<>();
            private SuggestionListViewHolder mHolder;

            @Override
            public void inflate() {
                provider.whenLoaded(
                        (holder) -> {
                            OmniboxSuggestionsDropdown dropdown = holder.dropdown;
                            if (dropdown != null
                                    && dropdown.getAdapter() != null
                                    && dropdown.getAdapter()
                                            instanceof OmniboxSuggestionsDropdownAdapter) {
                                addTypes((OmniboxSuggestionsDropdownAdapter) dropdown.getAdapter());
                                mHolder = holder;
                                for (int i = 0; i < mCallbacks.size(); i++) {
                                    mCallbacks.get(i).onResult(holder);
                                }
                                mCallbacks = null;
                            }
                        });
                provider.inflate();
            }

            private void addTypes(OmniboxSuggestionsDropdownAdapter adapter) {
                adapter.registerType(
                        BraveOmniboxSuggestionUiType.BRAVE_SEARCH_PROMO_BANNER,
                        parent ->
                                LayoutInflater.from(parent.getContext())
                                        .inflate(R.layout.omnibox_brave_search_banner, null),
                        BraveSearchBannerViewBinder::bind);

                adapter.registerType(
                        BraveOmniboxSuggestionUiType.BRAVE_LEO_SUGGESTION,
                        parent ->
                                new BaseSuggestionView<View>(
                                        parent.getContext(), R.layout.omnibox_basic_suggestion),
                        new BaseSuggestionViewBinder<View>(BraveLeoSuggestionViewBinder::bind));
            }

            @Override
            public void whenLoaded(Callback<SuggestionListViewHolder> callback) {
                if (mHolder != null) {
                    callback.onResult(mHolder);
                    return;
                }
                mCallbacks.add(callback);
            }
        };
    }
}
