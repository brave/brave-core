/**
 * Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.omnibox.suggestions;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewStub;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import org.chromium.base.Callback;
import org.chromium.base.StrictModeContext;
import org.chromium.base.jank_tracker.JankTracker;
import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.Supplier;
import org.chromium.chrome.browser.omnibox.LocationBarDataProvider;
import org.chromium.chrome.browser.omnibox.R;
import org.chromium.chrome.browser.omnibox.UrlBarEditingTextStateProvider;
import org.chromium.chrome.browser.omnibox.suggestions.SuggestionListViewBinder.SuggestionListViewHolder;
import org.chromium.chrome.browser.omnibox.suggestions.answer.AnswerSuggestionViewBinder;
import org.chromium.chrome.browser.omnibox.suggestions.base.BaseSuggestionView;
import org.chromium.chrome.browser.omnibox.suggestions.base.BaseSuggestionViewBinder;
import org.chromium.chrome.browser.omnibox.suggestions.basic.BasicSuggestionProcessor.BookmarkState;
import org.chromium.chrome.browser.omnibox.suggestions.basic.SuggestionViewViewBinder;
import org.chromium.chrome.browser.omnibox.suggestions.brave_search.BraveSearchBannerViewBinder;
import org.chromium.chrome.browser.omnibox.suggestions.carousel.BaseCarouselSuggestionViewBinder;
import org.chromium.chrome.browser.omnibox.suggestions.editurl.EditUrlSuggestionView;
import org.chromium.chrome.browser.omnibox.suggestions.editurl.EditUrlSuggestionViewBinder;
import org.chromium.chrome.browser.omnibox.suggestions.entity.EntitySuggestionViewBinder;
import org.chromium.chrome.browser.omnibox.suggestions.header.HeaderView;
import org.chromium.chrome.browser.omnibox.suggestions.header.HeaderViewBinder;
import org.chromium.chrome.browser.omnibox.suggestions.mostvisited.ExploreIconProvider;
import org.chromium.chrome.browser.omnibox.suggestions.mostvisited.MostVisitedTilesProcessor;
import org.chromium.chrome.browser.omnibox.suggestions.pedal.PedalSuggestionView;
import org.chromium.chrome.browser.omnibox.suggestions.pedal.PedalSuggestionViewBinder;
import org.chromium.chrome.browser.omnibox.suggestions.tail.TailSuggestionView;
import org.chromium.chrome.browser.omnibox.suggestions.tail.TailSuggestionViewBinder;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.share.ShareDelegate;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.TabWindowManager;
import org.chromium.ui.ViewProvider;
import org.chromium.ui.modaldialog.ModalDialogManager;
import org.chromium.ui.modelutil.MVCListAdapter;
import org.chromium.ui.modelutil.MVCListAdapter.ModelList;

import java.util.ArrayList;
import java.util.List;

public class BraveAutocompleteCoordinator extends AutocompleteCoordinator {
    private @Nullable OmniboxSuggestionsDropdown mDropdown;
    private final ViewGroup mParent;

    public BraveAutocompleteCoordinator(@NonNull ViewGroup parent,
            @NonNull AutocompleteDelegate delegate,
            @NonNull OmniboxSuggestionsDropdownEmbedder dropdownEmbedder,
            @NonNull UrlBarEditingTextStateProvider urlBarEditingTextProvider,
            @NonNull Supplier<ModalDialogManager> modalDialogManagerSupplier,
            @NonNull Supplier<Tab> activityTabSupplier,
            @Nullable Supplier<ShareDelegate> shareDelegateSupplier,
            @NonNull LocationBarDataProvider locationBarDataProvider,
            @NonNull ObservableSupplier<Profile> profileObservableSupplier,
            @NonNull Callback<Tab> bringToForegroundCallback,
            @NonNull Supplier<TabWindowManager> tabWindowManagerSupplier,
            @NonNull BookmarkState bookmarkState, @NonNull JankTracker jankTracker,
            @NonNull ExploreIconProvider exploreIconProvider,
            @NonNull OmniboxPedalDelegate omniboxPedalDelegate) {
        super(parent, delegate, dropdownEmbedder, urlBarEditingTextProvider,
                modalDialogManagerSupplier, activityTabSupplier, shareDelegateSupplier,
                locationBarDataProvider, profileObservableSupplier, bringToForegroundCallback,
                tabWindowManagerSupplier, bookmarkState, jankTracker, exploreIconProvider,
                omniboxPedalDelegate);

        mParent = parent;
    }

    public ViewProvider<SuggestionListViewHolder> createViewProvider(
            Context context, MVCListAdapter.ModelList modelList) {
        return new ViewProvider<SuggestionListViewHolder>() {
            private List<Callback<SuggestionListViewHolder>> mCallbacks = new ArrayList<>();
            private SuggestionListViewHolder mHolder;
            @Override
            public void inflate() {
                OmniboxSuggestionsDropdown dropdown;
                try (StrictModeContext ignored = StrictModeContext.allowDiskReads()) {
                    dropdown = new OmniboxSuggestionsDropdown(context);
                }

                // Start with visibility GONE to ensure that show() is called.
                // http://crbug.com/517438
                dropdown.getViewGroup().setVisibility(View.GONE);
                dropdown.getViewGroup().setClipToPadding(false);

                OmniboxSuggestionsDropdownAdapter adapter =
                        new OmniboxSuggestionsDropdownAdapter(modelList);
                dropdown.setAdapter(adapter);

                // Note: clang-format does a bad job formatting lambdas so we turn it off here.
                // clang-format off
                // Register a view type for a default omnibox suggestion.
                adapter.registerType(
                        OmniboxSuggestionUiType.DEFAULT,
                        parent -> new BaseSuggestionView<View>(
                                parent.getContext(), R.layout.omnibox_basic_suggestion),
                        new BaseSuggestionViewBinder<View>(SuggestionViewViewBinder::bind));

                adapter.registerType(
                        OmniboxSuggestionUiType.EDIT_URL_SUGGESTION,
                        parent -> new EditUrlSuggestionView(parent.getContext()),
                        new EditUrlSuggestionViewBinder());

                adapter.registerType(
                        OmniboxSuggestionUiType.ANSWER_SUGGESTION,
                        parent -> new BaseSuggestionView<View>(
                                parent.getContext(), R.layout.omnibox_answer_suggestion),
                        new BaseSuggestionViewBinder<View>(AnswerSuggestionViewBinder::bind));

                adapter.registerType(
                        OmniboxSuggestionUiType.ENTITY_SUGGESTION,
                        parent -> new BaseSuggestionView<View>(
                                parent.getContext(), R.layout.omnibox_entity_suggestion),
                        new BaseSuggestionViewBinder<View>(EntitySuggestionViewBinder::bind));

                adapter.registerType(
                        OmniboxSuggestionUiType.TAIL_SUGGESTION,
                        parent -> new BaseSuggestionView<TailSuggestionView>(
                                new TailSuggestionView(parent.getContext())),
                        new BaseSuggestionViewBinder<TailSuggestionView>(
                                TailSuggestionViewBinder::bind));

                adapter.registerType(
                        OmniboxSuggestionUiType.CLIPBOARD_SUGGESTION,
                        parent -> new BaseSuggestionView<View>(
                                parent.getContext(), R.layout.omnibox_basic_suggestion),
                        new BaseSuggestionViewBinder<View>(SuggestionViewViewBinder::bind));

                adapter.registerType(
                        OmniboxSuggestionUiType.TILE_NAVSUGGEST,
                        MostVisitedTilesProcessor::createView,
                        BaseCarouselSuggestionViewBinder::bind);

                adapter.registerType(
                        BraveOmniboxSuggestionUiType.HEADER,
                        parent -> new HeaderView(parent.getContext()),
                        HeaderViewBinder::bind);

                adapter.registerType(
                        OmniboxSuggestionUiType.PEDAL_SUGGESTION,
                        parent -> new PedalSuggestionView<View>(
                                parent.getContext(), R.layout.omnibox_basic_suggestion),
                        new PedalSuggestionViewBinder<View>(SuggestionViewViewBinder::bind));

                adapter.registerType(BraveOmniboxSuggestionUiType.BRAVE_SEARCH_PROMO_BANNER,
                    parent -> LayoutInflater.from(parent.getContext()).inflate(R.layout.omnibox_brave_search_banner, null),
                        BraveSearchBannerViewBinder::bind);

                // clang-format on

                ViewGroup container = (ViewGroup) ((ViewStub) mParent.getRootView().findViewById(
                                                           R.id.omnibox_results_container_stub))
                                              .inflate();

                mHolder = new SuggestionListViewHolder(container, dropdown);
                for (int i = 0; i < mCallbacks.size(); i++) {
                    mCallbacks.get(i).onResult(mHolder);
                }
                mCallbacks = null;
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
