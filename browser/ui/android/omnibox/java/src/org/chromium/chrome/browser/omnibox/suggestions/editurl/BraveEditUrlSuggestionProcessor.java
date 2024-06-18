/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.omnibox.suggestions.editurl;

import android.content.Context;

import androidx.annotation.NonNull;

import org.chromium.base.supplier.Supplier;
import org.chromium.chrome.browser.omnibox.styles.OmniboxImageSupplier;
import org.chromium.chrome.browser.omnibox.suggestions.SuggestionHost;
import org.chromium.chrome.browser.share.ShareDelegate;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.components.omnibox.AutocompleteMatch;
import org.chromium.components.omnibox.OmniboxSuggestionType;

import java.util.Optional;

public class BraveEditUrlSuggestionProcessor extends EditUrlSuggestionProcessor {
    private final @NonNull Supplier<Tab> mTabSupplier;

    public BraveEditUrlSuggestionProcessor(
            Context context,
            SuggestionHost suggestionHost,
            Optional<OmniboxImageSupplier> imageSupplier,
            Supplier<Tab> tabSupplier,
            Supplier<ShareDelegate> shareDelegateSupplier) {
        super(context, suggestionHost, imageSupplier, tabSupplier, shareDelegateSupplier);

        mTabSupplier = tabSupplier;
    }

    @Override
    public boolean doesProcessSuggestion(@NonNull AutocompleteMatch suggestion, int position) {
        // Edit url suggestion only applicable to the first entry.
        Tab activeTab = mTabSupplier.get();
        if (position == 0
                && activeTab != null
                && (suggestion.getType() == OmniboxSuggestionType.URL_WHAT_YOU_TYPED
                                && tabMatchesSuggestion(activeTab, suggestion)
                        || suggestion.getUrl().equals(activeTab.getUrl()))) {
            // Show edit url suggestion for typed URLs.
            // If url hasn't changed we still want to show the edit url suggestion.
            return true;
        }

        return super.doesProcessSuggestion(suggestion, position);
    }

    public AutocompleteMatch maybeUpdateSuggestion(AutocompleteMatch suggestion) {
        Tab activeTab = mTabSupplier.get();
        if (suggestion.getType() == OmniboxSuggestionType.URL_WHAT_YOU_TYPED
                && activeTab != null
                && tabMatchesSuggestion(activeTab, suggestion)) {
            // For manually typed URLs update suggestion URL with properly resolved one from Tab.
            return new AutocompleteMatch(
                    OmniboxSuggestionType.URL_WHAT_YOU_TYPED,
                    suggestion.getSubtypes(),
                    suggestion.isSearchSuggestion(),
                    suggestion.getRelevance(),
                    suggestion.getTransition(),
                    suggestion.getDisplayText(),
                    suggestion.getDisplayTextClassifications(),
                    suggestion.getDescription(),
                    suggestion.getDescriptionClassifications(),
                    suggestion.getAnswer(),
                    null,
                    suggestion.getFillIntoEdit(),
                    activeTab.getUrl(),
                    suggestion.getImageUrl(),
                    suggestion.getImageDominantColor(),
                    suggestion.isDeletable(),
                    suggestion.getPostContentType(),
                    suggestion.getPostData(),
                    suggestion.getGroupId(),
                    suggestion.getClipboardImageData(),
                    suggestion.hasTabMatch(),
                    suggestion.getActions(),
                    suggestion.allowedToBeDefaultMatch(),
                    suggestion.getInlineAutocompletion(),
                    suggestion.getAdditionalText());
        }
        return suggestion;
    }

    private boolean tabMatchesSuggestion(Tab tab, AutocompleteMatch suggestion) {
        return tab.getUrl().isValid()
                && suggestion.getUrl().isValid()
                && tab.getUrl().domainIs(suggestion.getUrl().getHost())
                && tab.getUrl().getPath().equals(suggestion.getUrl().getPath());
    }
}
