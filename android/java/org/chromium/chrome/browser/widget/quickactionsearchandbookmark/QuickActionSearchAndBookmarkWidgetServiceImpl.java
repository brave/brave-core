/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.widget.quickactionsearchandbookmark;

import static org.chromium.build.NullUtil.assumeNonNull;

import android.content.Intent;
import android.widget.RemoteViewsService.RemoteViewsFactory;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.base.SplitCompatRemoteViewsService;

/** Implementation for {@link QuickActionSearchAndBookmarkWidgetService}. */
@NullMarked
public class QuickActionSearchAndBookmarkWidgetServiceImpl
        extends SplitCompatRemoteViewsService.Impl {
    @Override
    public @Nullable RemoteViewsFactory onGetViewFactory(Intent intent) {
        return new BookmarkRemoteViewsFactory(
                assumeNonNull(getService()).getApplicationContext(), intent);
    }
}
