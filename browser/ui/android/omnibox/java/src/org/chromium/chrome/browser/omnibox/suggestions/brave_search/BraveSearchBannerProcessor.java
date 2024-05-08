/**
 * Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.omnibox.suggestions.brave_search;

import android.content.Context;

import org.chromium.chrome.browser.omnibox.R;
import org.chromium.chrome.browser.omnibox.UrlBarEditingTextStateProvider;
import org.chromium.chrome.browser.omnibox.suggestions.AutocompleteDelegate;
import org.chromium.chrome.browser.omnibox.suggestions.BraveOmniboxSuggestionUiType;
import org.chromium.chrome.browser.omnibox.suggestions.BraveSuggestionHost;
import org.chromium.chrome.browser.omnibox.suggestions.DropdownItemProcessor;
import org.chromium.chrome.browser.omnibox.suggestions.OmniboxLoadUrlParams;
import org.chromium.ui.base.PageTransition;
import org.chromium.ui.modelutil.PropertyModel;

/** A class that handles model and view creation for the suggestion brave search banner. */
public class BraveSearchBannerProcessor implements DropdownItemProcessor {
    private final BraveSuggestionHost mSuggestionHost;
    private final int mMinimumHeight;
    private final UrlBarEditingTextStateProvider mUrlBarEditingTextProvider;
    private final AutocompleteDelegate mUrlBarDelegate;

    /**
     * @param context An Android context.
     * @param suggestionHost A handle to the object using the suggestions.
     */
    public BraveSearchBannerProcessor(Context context, BraveSuggestionHost suggestionHost,
            UrlBarEditingTextStateProvider editingTextProvider, AutocompleteDelegate urlDelegate) {
        mSuggestionHost = suggestionHost;
        mUrlBarEditingTextProvider = editingTextProvider;
        mUrlBarDelegate = urlDelegate;
        mMinimumHeight = context.getResources().getDimensionPixelSize(
                R.dimen.omnibox_brave_search_banner_height);
    }

    public void populateModel(final PropertyModel model) {
        model.set(
                BraveSearchBannerProperties.DELEGATE,
                new BraveSearchBannerProperties.Delegate() {
                    @Override
                    public void onPositiveClicked() {
                        mUrlBarDelegate.loadUrl(
                                new OmniboxLoadUrlParams.Builder(
                                                "https://search.brave.com/search?q="
                                                        + mUrlBarEditingTextProvider
                                                                .getTextWithoutAutocomplete()
                                                        + "&action=makeDefault",
                                                PageTransition.LINK)
                                        .setInputStartTimestamp(System.currentTimeMillis())
                                        .setOpenInNewTab(false)
                                        .build());
                    }

                    @Override
                    public void onNegativeClicked() {
                        mSuggestionHost.removeBraveSearchSuggestion();
                    }
                });
    }

    @Override
    public int getViewTypeId() {
        return BraveOmniboxSuggestionUiType.BRAVE_SEARCH_PROMO_BANNER;
    }

    @Override
    public int getMinimumViewHeight() {
        return mMinimumHeight;
    }

    @Override
    public PropertyModel createModel() {
        return new PropertyModel(BraveSearchBannerProperties.ALL_KEYS);
    }

    @Override
    public void onOmniboxSessionStateChange(boolean activated) {}

    @Override
    public void onNativeInitialized() {}
}
