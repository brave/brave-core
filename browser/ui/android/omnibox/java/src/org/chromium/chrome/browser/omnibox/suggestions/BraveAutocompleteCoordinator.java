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

import androidx.annotation.CallSuper;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import org.chromium.base.BraveReflectionUtil;
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

import java.lang.reflect.InvocationTargetException;
import java.util.ArrayList;
import java.util.List;

public class BraveAutocompleteCoordinator {
    public ViewProvider<SuggestionListViewHolder> createViewProvider(Context context,
            MVCListAdapter.ModelList modelList, LocationBarDataProvider locationBarDataProvider) {
        ViewProvider<SuggestionListViewHolder> provider =
                (ViewProvider<SuggestionListViewHolder>) BraveReflectionUtil.InvokeMethod(
                        AutocompleteCoordinator.class, this, "createViewProvider", Context.class,
                        context, MVCListAdapter.ModelList.class, modelList,
                        LocationBarDataProvider.class, locationBarDataProvider);

        return new ViewProvider<SuggestionListViewHolder>() {
            private List<Callback<SuggestionListViewHolder>> mCallbacks = new ArrayList<>();
            private SuggestionListViewHolder mHolder;
            @Override
            public void inflate() {
                provider.whenLoaded((holder) -> {
                    OmniboxSuggestionsDropdown dropdown = holder.dropdown;
                    if (dropdown != null && dropdown.getAdapter() != null
                            && dropdown.getAdapter() instanceof OmniboxSuggestionsDropdownAdapter) {
                        ((OmniboxSuggestionsDropdownAdapter) dropdown.getAdapter())
                                .registerType(
                                        BraveOmniboxSuggestionUiType.BRAVE_SEARCH_PROMO_BANNER,
                                        parent
                                        -> LayoutInflater.from(parent.getContext())
                                                   .inflate(R.layout.omnibox_brave_search_banner,
                                                           null),
                                        BraveSearchBannerViewBinder::bind);

                        mHolder = holder;
                        for (int i = 0; i < mCallbacks.size(); i++) {
                            mCallbacks.get(i).onResult(holder);
                        }
                        mCallbacks = null;
                    }
                });
                provider.inflate();
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
