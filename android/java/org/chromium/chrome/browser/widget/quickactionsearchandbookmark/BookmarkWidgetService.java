/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.widget.quickactionsearchandbookmark;

import android.content.Intent;
import android.widget.RemoteViewsService;

import org.chromium.build.annotations.NullMarked;

/** RemoteViewsService for the bookmark GridView in the Quick Action widget. */
@NullMarked
public class BookmarkWidgetService extends RemoteViewsService {
    @Override
    public RemoteViewsFactory onGetViewFactory(Intent intent) {
        return new BookmarkRemoteViewsFactory(this.getApplicationContext(), intent);
    }
}
