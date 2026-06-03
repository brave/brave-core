/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import org.chromium.base.Callback;
import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.partnerbookmarks.PartnerBookmark;

import java.util.NoSuchElementException;

@NullMarked
public class BraveAppHooks extends AppHooks {
    /** Async fetch the iterator of partner bookmarks (or null if not available). */
    @Override
    public void requestPartnerBookmarkIterator(
            Callback<PartnerBookmark.BookmarkIterator> callback) {
        // Brave has no partner bookmarks; signal completion with an empty iterator so the
        // PartnerBookmarksShim is marked as loaded and the bookmark model can finish loading.
        callback.onResult(
                new PartnerBookmark.BookmarkIterator() {
                    @Override
                    public void close() {}

                    @Override
                    public boolean hasNext() {
                        return false;
                    }

                    @Override
                    public PartnerBookmark next() {
                        throw new NoSuchElementException();
                    }
                });
    }
}
