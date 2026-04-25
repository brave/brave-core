/**
 * Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.omnibox.suggestions.brave_search;

import org.chromium.chrome.browser.omnibox.suggestions.SuggestionCommonProperties;
import org.chromium.ui.modelutil.PropertyKey;
import org.chromium.ui.modelutil.PropertyModel;
import org.chromium.ui.modelutil.PropertyModel.WritableObjectPropertyKey;

/** The properties associated with the brave search banner suggestions. */
public class BraveSearchBannerProperties {
    /** Interface that receives events from brave search banner view. */
    interface Delegate {
        void onPositiveClicked();

        void onNegativeClicked();
    }

    public static final WritableObjectPropertyKey<Delegate> DELEGATE =
            new WritableObjectPropertyKey<>();

    public static final PropertyKey[] ALL_UNIQUE_KEYS = new PropertyKey[] {DELEGATE};

    public static final PropertyKey[] ALL_KEYS =
            PropertyModel.concatKeys(ALL_UNIQUE_KEYS, SuggestionCommonProperties.ALL_KEYS);
}
