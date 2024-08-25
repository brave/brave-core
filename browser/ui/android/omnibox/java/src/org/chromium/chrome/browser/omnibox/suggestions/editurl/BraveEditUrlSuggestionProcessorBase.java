/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.omnibox.suggestions.editurl;

import android.content.Context;

import org.chromium.base.BraveReflectionUtil;
import org.chromium.chrome.browser.omnibox.styles.OmniboxImageSupplier;
import org.chromium.chrome.browser.omnibox.suggestions.SuggestionHost;
import org.chromium.chrome.browser.omnibox.suggestions.base.BaseSuggestionViewProcessor;
import org.chromium.components.omnibox.AutocompleteMatch;

import java.util.Optional;

public abstract class BraveEditUrlSuggestionProcessorBase extends BaseSuggestionViewProcessor {
    public BraveEditUrlSuggestionProcessorBase(
            Context context,
            SuggestionHost suggestionHost,
            Optional<OmniboxImageSupplier> imageSupplier) {
        super(context, suggestionHost, imageSupplier);
    }

    /*
     * Calls to the upstream's `EditUrlSuggestionProcessor.onCopyLink` will be redirected here via bytecode.
     * Here we call the upstream's `EditUrlSuggestionProcessor.onCopyLink` with possibly changed suggestion.
     * See comment to `BraveEditUrlSuggestionProcessor.maybeUpdateSuggestionForCopyLink` for more details on why it may be changed.
     */
    public void onCopyLink(AutocompleteMatch suggestion) {
        BraveReflectionUtil.invokeMethod(
                EditUrlSuggestionProcessor.class,
                this,
                "onCopyLink",
                AutocompleteMatch.class,
                maybeUpdateSuggestionForCopyLink(suggestion));
    }

    public AutocompleteMatch maybeUpdateSuggestionForCopyLink(AutocompleteMatch suggestion) {
        assert false : "This method should be overridden by BraveEditUrlSuggestionProcessor";
        return suggestion;
    }
}
