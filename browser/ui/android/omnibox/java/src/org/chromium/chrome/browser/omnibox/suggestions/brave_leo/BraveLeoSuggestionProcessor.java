/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.omnibox.suggestions.brave_leo;

import androidx.annotation.NonNull;

import org.chromium.chrome.browser.omnibox.R;
import org.chromium.chrome.browser.omnibox.UrlBarEditingTextStateProvider;
import org.chromium.chrome.browser.omnibox.styles.OmniboxDrawableState;
import org.chromium.chrome.browser.omnibox.styles.SuggestionSpannable;
import org.chromium.chrome.browser.omnibox.suggestions.AutocompleteUIContext;
import org.chromium.chrome.browser.omnibox.suggestions.BraveLeoAutocompleteDelegate;
import org.chromium.chrome.browser.omnibox.suggestions.BraveOmniboxSuggestionUiType;
import org.chromium.chrome.browser.omnibox.suggestions.base.BaseSuggestionViewProcessor;
import org.chromium.chrome.browser.omnibox.suggestions.base.BaseSuggestionViewProperties;
import org.chromium.chrome.browser.omnibox.suggestions.basic.SuggestionViewProperties;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.components.omnibox.AutocompleteMatch;
import org.chromium.ui.modelutil.PropertyModel;

import java.util.function.Supplier;

/** A class that handles model and view creation for the Brave Leo suggestion. */
public class BraveLeoSuggestionProcessor extends BaseSuggestionViewProcessor {
    private final UrlBarEditingTextStateProvider mUrlBarEditingTextProvider;
    private BraveLeoAutocompleteDelegate mBraveLeoAutocompleteDelegate;
    private final String mAskLeo;
    private final @NonNull Supplier<Tab> mActivityTabSupplier;

    public BraveLeoSuggestionProcessor(AutocompleteUIContext uiContext) {
        super(uiContext);
        mActivityTabSupplier = uiContext.activityTabSupplier;
        mUrlBarEditingTextProvider = uiContext.textProvider;
        mAskLeo = uiContext.context.getResources().getString(R.string.ask_leo_auto_suggestion);
    }

    public void setBraveLeoAutocompleteDelegate(BraveLeoAutocompleteDelegate delegate) {
        mBraveLeoAutocompleteDelegate = delegate;
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
                        mBraveLeoAutocompleteDelegate.openLeoQuery(
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
