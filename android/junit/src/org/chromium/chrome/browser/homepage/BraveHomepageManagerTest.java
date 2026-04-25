/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.homepage;

import static org.mockito.Mockito.when;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.browser.bookmarks.BookmarkModel;
import org.chromium.chrome.browser.preferences.ChromePreferenceKeys;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.components.bookmarks.BookmarkId;
import org.chromium.components.bookmarks.BookmarkType;
import org.chromium.url.GURL;

/** Unit tests for {@link BraveHomepageManager}. */
@RunWith(BaseRobolectricTestRunner.class)
public class BraveHomepageManagerTest {
    private static final long TEST_BOOKMARK_ID = 42;
    private static final BookmarkId MOBILE_BOOKMARKS_REASSIGNED_ID =
            new BookmarkId(TEST_BOOKMARK_ID, BookmarkType.NORMAL);

    @Rule public final MockitoRule mMockitoRule = MockitoJUnit.rule();
    @Mock private BookmarkModel mBookmarkModel;
    @Mock private Profile mProfile;

    @Before
    public void setUp() {
        BookmarkModel.setInstanceForTesting(mBookmarkModel);

        when(mBookmarkModel.getMobileFolderId()).thenReturn(MOBILE_BOOKMARKS_REASSIGNED_ID);
        when(mBookmarkModel.isBookmarkModelLoaded()).thenReturn(true);

        ProfileManager.setLastUsedProfileForTesting(mProfile);
    }

    @Test
    public void testMobileBookmarksHomepageGurl() {
        final GURL gurlBookmarkId3 = new GURL("chrome-native://bookmarks/folder/3");
        ChromeSharedPreferences.getInstance()
                .writeString(
                        ChromePreferenceKeys.HOMEPAGE_CUSTOM_GURL, gurlBookmarkId3.serialize());

        ProfileManager.onProfileAdded(ProfileManager.getLastUsedRegularProfile());

        HomepageManager homepageManager = HomepageManager.getInstance();
        GURL url = homepageManager.getPrefHomepageCustomGurl();
        Assert.assertEquals("chrome-native://bookmarks/folder/" + TEST_BOOKMARK_ID, url.getSpec());
    }
}
