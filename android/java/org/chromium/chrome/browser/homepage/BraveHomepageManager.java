/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.homepage;

import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.bookmarks.BookmarkModel;
import org.chromium.chrome.browser.partnercustomizations.CloseBraveManager;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.url.GURL;

// see org.brave.bytecode.BraveHomepageManagerClassAdapter
@NullMarked
public class BraveHomepageManager extends HomepageManager {
    private static final String MOBILE_BOOKMARKS_PATH = "chrome-native://bookmarks/folder/3";

    @Override
    public boolean shouldCloseAppWithZeroTabs() {
        return CloseBraveManager.shouldCloseAppWithZeroTabs();
    }

    @Override
    public GURL getPrefHomepageCustomGurl() {
        GURL originalUrl = super.getPrefHomepageCustomGurl();
        if (originalUrl.getSpec().equals(MOBILE_BOOKMARKS_PATH) && ProfileManager.isInitialized()) {
            // Bookmark item with id 3 used to be a 'Mobile bookmarks' folder.
            // Now bookmark ids are reassigned at `BookmarkCodec::Decode`, so
            // id 3 have another random bookmark and 'Mobile bookmarks' can have different id each
            // time. This always happens after cr141 => cr142 upgrade because
            // stored_sha256_checksum_
            // is empty. IDs may also be reassigned at any point, but Home page has option only for
            // Mobile bookmarks folder, so get it's id dynamically.
            BookmarkModel bookmarkModel =
                    BookmarkModel.getForProfile(ProfileManager.getLastUsedRegularProfile());
            if (bookmarkModel.isBookmarkModelLoaded()
                    && bookmarkModel.getMobileFolderId() != null) {
                return new GURL(
                        "chrome-native://bookmarks/folder/"
                                + bookmarkModel.getMobileFolderId().getId());
            }
        }
        return originalUrl;
    }
}
