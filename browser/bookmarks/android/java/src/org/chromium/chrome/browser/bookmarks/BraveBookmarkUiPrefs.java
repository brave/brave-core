/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.bookmarks;

import org.chromium.base.shared_preferences.SharedPreferencesManager;
import org.chromium.chrome.browser.preferences.ChromePreferenceKeys;

/** Brave's extension for BookmarkUiPrefs. */
public class BraveBookmarkUiPrefs extends BookmarkUiPrefs {
    private final SharedPreferencesManager mPrefsManager;

    public BraveBookmarkUiPrefs(SharedPreferencesManager prefsManager) {
        super(prefsManager);

        mPrefsManager = prefsManager;
    }

    /** Returns how the bookmark rows should be displayed, doesn't write anything to prefs. */
    @Override
    public @BookmarkRowDisplayPref int getBookmarkRowDisplayPref() {
        return mPrefsManager.readInt(
                ChromePreferenceKeys.BOOKMARKS_VISUALS_PREF, BookmarkRowDisplayPref.COMPACT);
    }
}
