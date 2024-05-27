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
        if (position == 0) {
            // Show edit url suggestion for typed URLs.
            if (suggestion.getType() == OmniboxSuggestionType.URL_WHAT_YOU_TYPED) {
                return true;
            }
            // If url hasn't changed we still want to show the edit url suggestion.
            Tab activeTab = mTabSupplier.get();
            if (activeTab != null && suggestion.getUrl().equals(activeTab.getUrl())) {
                return true;
            }
        }

        return super.doesProcessSuggestion(suggestion, position);
    }
}
