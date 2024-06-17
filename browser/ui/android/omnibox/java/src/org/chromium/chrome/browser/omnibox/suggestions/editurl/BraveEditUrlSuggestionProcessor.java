/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.omnibox.suggestions.editurl;

import android.content.Context;

import androidx.annotation.NonNull;

import org.chromium.base.supplier.Supplier;
import org.chromium.chrome.browser.history_clusters.HistoryClustersTabHelper;
import org.chromium.chrome.browser.omnibox.styles.OmniboxImageSupplier;
import org.chromium.chrome.browser.omnibox.suggestions.SuggestionHost;
import org.chromium.chrome.browser.share.ShareDelegate;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.components.omnibox.AutocompleteMatch;
import org.chromium.components.omnibox.OmniboxSuggestionType;
import org.chromium.ui.base.Clipboard;

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

    protected void onCopyLink(AutocompleteMatch suggestion) {
        Tab activeTab = mTabSupplier.get();
        if (suggestion.getType() == OmniboxSuggestionType.URL_WHAT_YOU_TYPED
                && activeTab != null
                && tabMatchesSuggestion(activeTab, suggestion)) {
            // For manually typed URLs, copy properly resolved URL from Tab instead of the
            // suggestion URL.
            HistoryClustersTabHelper.onCurrentTabUrlCopied(activeTab.getWebContents());
            Clipboard.getInstance().copyUrlToClipboard(activeTab.getUrl());
            return;
        }
        // Otherwise fallback to the default behavior.
        HistoryClustersTabHelper.onCurrentTabUrlCopied(mTabSupplier.get().getWebContents());
        Clipboard.getInstance().copyUrlToClipboard(suggestion.getUrl());
    }

    private boolean tabMatchesSuggestion(Tab tab, AutocompleteMatch suggestion) {
        return tab.getUrl().isValid()
                && suggestion.getUrl().isValid()
                && tab.getUrl().domainIs(suggestion.getUrl().getHost())
                && tab.getUrl().getPath().equals(suggestion.getUrl().getPath());
    }
}
