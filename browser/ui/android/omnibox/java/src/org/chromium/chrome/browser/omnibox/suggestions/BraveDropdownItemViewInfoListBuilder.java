/**
 * Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.omnibox.suggestions;

import android.content.Context;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.Px;

import org.chromium.base.ContextUtils;
import org.chromium.base.supplier.Supplier;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.omnibox.OmniboxPrefManager;
import org.chromium.chrome.browser.omnibox.R;
import org.chromium.chrome.browser.omnibox.UrlBarEditingTextStateProvider;
import org.chromium.chrome.browser.omnibox.suggestions.basic.BasicSuggestionProcessor.BookmarkState;
import org.chromium.chrome.browser.omnibox.suggestions.brave_search.BraveSearchBannerProcessor;
import org.chromium.chrome.browser.omnibox.suggestions.mostvisited.ExploreIconProvider;
import org.chromium.chrome.browser.search_engines.settings.BraveSearchEngineAdapter;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.components.omnibox.AutocompleteMatch;
import org.chromium.components.omnibox.AutocompleteResult;
import org.chromium.ui.modelutil.PropertyModel;

import java.util.Arrays;
import java.util.List;
import java.util.Locale;

class BraveDropdownItemViewInfoListBuilder extends DropdownItemViewInfoListBuilder {
    private @Nullable BraveSearchBannerProcessor mBraveSearchBannerProcessor;
    private UrlBarEditingTextStateProvider mUrlBarEditingTextProvider;
    private @NonNull Supplier<Tab> mActivityTabSupplier;
    private List<SuggestionProcessor> mPriorityOrderedSuggestionProcessors;
    private static final List<String> mBraveSearchEngineDefaultRegions =
            Arrays.asList("CA", "DE", "FR", "GB", "US", "AT", "ES", "MX");
    @Px
    private static final int DROPDOWN_HEIGHT_UNKNOWN = -1;
    private static final int DEFAULT_SIZE_OF_VISIBLE_GROUP = 5;

    @Px
    private int mDropdownHeight;
    @Px
    private int mCalculatedSuggestionsHeight;
    private Context mContext;

    BraveDropdownItemViewInfoListBuilder(@NonNull Supplier<Tab> tabSupplier,
            BookmarkState bookmarkState, @NonNull ExploreIconProvider exploreIconProvider,
            @NonNull OmniboxPedalDelegate omniboxPedalDelegate) {
        super(tabSupplier, bookmarkState, exploreIconProvider, omniboxPedalDelegate);
        mActivityTabSupplier = tabSupplier;
    }

    @Override
    void initDefaultProcessors(Context context, SuggestionHost host, AutocompleteDelegate delegate,
            UrlBarEditingTextStateProvider textProvider) {
        mContext = context;
        mUrlBarEditingTextProvider = textProvider;
        super.initDefaultProcessors(context, host, delegate, textProvider);
        if (host instanceof BraveSuggestionHost) {
            mBraveSearchBannerProcessor = new BraveSearchBannerProcessor(
                    context, (BraveSuggestionHost) host, textProvider, delegate);
        }
    }

    @Override
    void onUrlFocusChange(boolean hasFocus) {
        super.onUrlFocusChange(hasFocus);
        mBraveSearchBannerProcessor.onUrlFocusChange(hasFocus);
    }

    @Override
    void onNativeInitialized() {
        super.onNativeInitialized();
        mBraveSearchBannerProcessor.onNativeInitialized();
    }

    @Override
    int getVisibleSuggestionsCount(AutocompleteResult autocompleteResult) {
        // For cases where we don't know how many suggestions can fit in the visile screen area,
        // make an assumption regarding the group size.
        if (mDropdownHeight == DROPDOWN_HEIGHT_UNKNOWN) {
            return Math.min(
                    autocompleteResult.getSuggestionsList().size(), DEFAULT_SIZE_OF_VISIBLE_GROUP);
        }

        final List<AutocompleteMatch> suggestions = autocompleteResult.getSuggestionsList();

        @Px
        int calculatedSuggestionsHeight = 0;
        int lastVisibleIndex;
        for (lastVisibleIndex = 0; lastVisibleIndex < suggestions.size(); lastVisibleIndex++) {
            if (calculatedSuggestionsHeight >= mDropdownHeight) break;

            final AutocompleteMatch suggestion = suggestions.get(lastVisibleIndex);
            // We do not include suggestions with headers in partial grouping, so terminate early.
            if (suggestion.getGroupId() != AutocompleteMatch.INVALID_GROUP) {
                break;
            }

            final SuggestionProcessor processor =
                    getProcessorForSuggestion(suggestion, lastVisibleIndex);

            calculatedSuggestionsHeight += processor.getMinimumViewHeight();
        }

        mCalculatedSuggestionsHeight = calculatedSuggestionsHeight;

        return lastVisibleIndex;
    }

    /**
     * Search for Processor that will handle the supplied suggestion at specific position.
     *
     * @param suggestion The suggestion to be processed.
     * @param position Position of the suggestion in the list.
     */
    private SuggestionProcessor getProcessorForSuggestion(
            AutocompleteMatch suggestion, int position) {
        for (int index = 0; index < mPriorityOrderedSuggestionProcessors.size(); index++) {
            SuggestionProcessor processor = mPriorityOrderedSuggestionProcessors.get(index);
            if (processor.doesProcessSuggestion(suggestion, position)) return processor;
        }
        assert false : "No default handler for suggestions";
        return null;
    }

    @Override
    @NonNull
    List<DropdownItemViewInfo> buildDropdownViewInfoList(AutocompleteResult autocompleteResult) {
        mBraveSearchBannerProcessor.onSuggestionsReceived();
        List<DropdownItemViewInfo> viewInfoList =
                super.buildDropdownViewInfoList(autocompleteResult);

        if (isBraveSearchPromoBanner()) {
            mCalculatedSuggestionsHeight += mBraveSearchBannerProcessor.getMinimumViewHeight();
            int viewHeight = mContext.getResources().getDimensionPixelSize(
                    R.dimen.omnibox_suggestion_semicompact_height);
            while (mCalculatedSuggestionsHeight >= mDropdownHeight) {
                mCalculatedSuggestionsHeight -= viewHeight;
                viewInfoList.remove(viewInfoList.size() - 1);
            }

            final PropertyModel model = mBraveSearchBannerProcessor.createModel();
            mBraveSearchBannerProcessor.populateModel(model);
            viewInfoList.add(new DropdownItemViewInfo(mBraveSearchBannerProcessor, model,
                    BraveSearchBannerProcessor.BRAVE_SEARCH_PROMO_GROUP));
        }

        return viewInfoList;
    }

    private boolean isBraveSearchPromoBanner() {
        Tab activeTab = mActivityTabSupplier.get();
        if (ChromeFeatureList.isEnabled("BraveSearchOmniboxBanner")
                && mUrlBarEditingTextProvider != null
                && mUrlBarEditingTextProvider.getTextWithoutAutocomplete().length() > 0
                && activeTab != null && !activeTab.isIncognito()
                && mBraveSearchEngineDefaultRegions.contains(Locale.getDefault().getCountry())
                && !BraveSearchEngineAdapter.getDSEShortName(false).equals("Brave")
                && !OmniboxPrefManager.getInstance().isBraveSearchPromoBannerDismissed()
                && !OmniboxPrefManager.getInstance()
                            .isBraveSearchPromoBannerDismissedCurrentSession()) {
            long expiredDate =
                    OmniboxPrefManager.getInstance().getBraveSearchPromoBannerExpiredDate();

            if (expiredDate == 0) {
                OmniboxPrefManager.getInstance().setBraveSearchPromoBannerExpiredDate();
                return true;
            } else if (expiredDate > System.currentTimeMillis()) {
                return true;
            }
            return false;
        } else {
            return false;
        }
    }
}
