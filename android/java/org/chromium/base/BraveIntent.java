/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.base;

import android.content.Context;
import android.content.Intent;

import java.util.HashMap;

public class BraveIntent extends Intent {
    private static final String TAG = "BraveIntent";

    // Set of classes to override.
    private static final HashMap<String, String> sClasses;

    static {
        sClasses = new HashMap<String, String>();
        sClasses.put(
                "org.chromium.chrome.browser.app.bookmarks.BookmarkActivity",
                "org.chromium.chrome.browser.app.bookmarks.BraveBookmarkActivity");
    }

    public BraveIntent(Context packageContext, Class<?> cls) {
        super(packageContext, cls);

        maybeChangeClass(packageContext, cls);
    }

    private void maybeChangeClass(Context packageContext, Class<?> cls) {
        if (packageContext == null) {
            return;
        }

        if (sClasses.containsKey(cls.getName())) {
            setClassName(packageContext, sClasses.get(cls.getName()));
        }
    }
}
