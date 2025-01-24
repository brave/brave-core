/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.bookmarks;

import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.ui.base.WindowAndroid;

public class BraveBookmarkModel extends BookmarkModel {
    BraveBookmarkModel(long nativeBookmarkBridge, Profile profile) {
        super(nativeBookmarkBridge, profile);
    }

    public void importBookmarks(WindowAndroid windowAndroid, String importFilePath) {
        assert false : "importBookmarks should be redirected to BraveBookmarkBridge in bytecode!";
    }

    public void exportBookmarks(WindowAndroid windowAndroid, String exportFilePath) {
        assert false : "exportBookmarks should be redirected to BraveBookmarkBridge in bytecode!";
    }
}
