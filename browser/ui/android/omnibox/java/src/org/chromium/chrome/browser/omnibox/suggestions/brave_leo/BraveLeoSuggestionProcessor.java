/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.omnibox.suggestions.brave_leo;

import android.content.Context;

import androidx.annotation.NonNull;

import org.chromium.base.supplier.Supplier;
import org.chromium.chrome.browser.omnibox.R;
import org.chromium.chrome.browser.omnibox.UrlBarEditingTextStateProvider;
import org.chromium.chrome.browser.omnibox.styles.OmniboxDrawableState;
import org.chromium.chrome.browser.omnibox.styles.OmniboxImageSupplier;
import org.chromium.chrome.browser.omnibox.styles.SuggestionSpannable;
import org.chromium.chrome.browser.omnibox.suggestions.BraveLeoAutocompleteDelegate;
import org.chromium.chrome.browser.omnibox.suggestions.BraveOmniboxSuggestionUiType;
import org.chromium.chrome.browser.omnibox.suggestions.SuggestionHost;
import org.chromium.chrome.browser.omnibox.suggestions.base.BaseSuggestionViewProcessor;
import org.chromium.chrome.browser.omnibox.suggestions.base.BaseSuggestionViewProperties;
import org.chromium.chrome.browser.omnibox.suggestions.basic.SuggestionViewProperties;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.components.omnibox.AutocompleteMatch;
import org.chromium.ui.modelutil.PropertyModel;

import java.util.Optional;

/** A class that handles model and view creation for the Brave Leo suggestion. */
public class BraveLeoSuggestionProcessor extends BaseSuggestionViewProcessor {
    private final UrlBarEditingTextStateProvider mUrlBarEditingTextProvider;
    private final BraveLeoAutocompleteDelegate mDelegate;
    private final String mAskLeo;
    private @NonNull Supplier<Tab> mActivityTabSupplier;

    public BraveLeoSuggestionProcessor(
            Context context,
            SuggestionHost suggestionHost,
            UrlBarEditingTextStateProvider editingTextProvider,
            @NonNull Optional<OmniboxImageSupplier> imageSupplier,
            BraveLeoAutocompleteDelegate delegate,
            @NonNull Supplier<Tab> tabSupplier) {
        super(context, suggestionHost, imageSupplier);
        mActivityTabSupplier = tabSupplier;
        mUrlBarEditingTextProvider = editingTextProvider;
        mDelegate = delegate;
        mAskLeo = context.getResources().getString(R.string.ask_leo_auto_suggestion);
    }

    public void populateModel(final PropertyModel model) {
        model.set(
                BaseSuggestionViewProperties.ICON,
                OmniboxDrawableState.forSmallIcon(
                        mContext, R.drawable.ic_brave_ai_color, /* allowTint= */ false));
        model.set(
                SuggestionViewProperties.TEXT_LINE_1_TEXT,
                new SuggestionSpannable(mUrlBarEditingTextProvider.getTextWithoutAutocomplete()));
        model.set(SuggestionViewProperties.TEXT_LINE_2_TEXT, new SuggestionSpannable(mAskLeo));
        model.set(
                BaseSuggestionViewProperties.ON_CLICK,
                () -> {
                    Tab tab = mActivityTabSupplier.get();
                    if (tab != null) {
                        mDelegate.openLeoQuery(
                                tab.getWebContents(),
                                "",
                                mUrlBarEditingTextProvider.getTextWithoutAutocomplete());
                    }
                });
    }

    @Override
    public int getViewTypeId() {
        return BraveOmniboxSuggestionUiType.BRAVE_LEO_SUGGESTION;
    }

    @Override
    public PropertyModel createModel() {
        return new PropertyModel(SuggestionViewProperties.ALL_KEYS);
    }

    @Override
    public boolean doesProcessSuggestion(AutocompleteMatch suggestion, int position) {
        return true;
    }
}
