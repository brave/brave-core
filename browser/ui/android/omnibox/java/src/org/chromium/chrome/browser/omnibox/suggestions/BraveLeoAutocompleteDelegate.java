/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.omnibox.suggestions;

import org.chromium.content_public.browser.WebContents;

/**
 * Provides additional functionality to trigger and interact with Brave Leo autocomplete suggestion.
 */
public interface BraveLeoAutocompleteDelegate {
    boolean isLeoEnabled();

    boolean isAutoCompleteEnabled(WebContents webContents);

    void openLeoQuery(WebContents webContents, String conversationUuid, String query);
}
