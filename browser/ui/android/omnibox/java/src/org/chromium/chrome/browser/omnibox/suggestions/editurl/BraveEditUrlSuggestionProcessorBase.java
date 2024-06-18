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

    public void onCopyLink(AutocompleteMatch suggestion) {
        BraveReflectionUtil.InvokeMethod(
                EditUrlSuggestionProcessor.class,
                this,
                "onCopyLink",
                AutocompleteMatch.class,
                maybeUpdateSuggestion(suggestion));
    }

    public AutocompleteMatch maybeUpdateSuggestion(AutocompleteMatch suggestion) {
        assert false : "This method should be overridden by BraveEditUrlSuggestionProcessor";
        return suggestion;
    }
}
