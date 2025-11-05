/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.widget.quickactionsearchandbookmark;

import org.chromium.build.annotations.IdentifierNameString;
import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.base.SplitCompatRemoteViewsService;

/** See {@link QuickActionSearchAndBookmarkWidgetServiceImpl}. */
@NullMarked
public class QuickActionSearchAndBookmarkWidgetService extends SplitCompatRemoteViewsService {
    @SuppressWarnings("FieldCanBeFinal") // @IdentifierNameString requires non-final
    private static @IdentifierNameString String sImplClassName =
            "org.chromium.chrome.browser.widget.quickactionsearchandbookmark.QuickActionSearchAndBookmarkWidgetServiceImpl"; // presubmit: ignore-long-line

    public QuickActionSearchAndBookmarkWidgetService() {
        super(sImplClassName);
    }
}
