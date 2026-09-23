/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.native_page;

import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.tasks.tab_management.BraveTabUiFeatureUtilities;
import org.chromium.chrome.browser.ui.native_page.TouchEnabledDelegate;

/**
 * Brave's {@link ContextMenuManager}. Hides the "Open in new tab in group" item from native page
 * context menus (e.g. New Tab Page shortcut tiles, Recent tabs) when the Brave "Enable tab groups"
 * master switch is off. Instantiated in place of the upstream class via a plaster redirect.
 */
@NullMarked
public class BraveContextMenuManager extends ContextMenuManager {
    public BraveContextMenuManager(
            NativePageNavigationDelegate navigationDelegate,
            TouchEnabledDelegate touchEnabledDelegate,
            Runnable closeContextMenuCallback,
            String userActionPrefix) {
        super(navigationDelegate, touchEnabledDelegate, closeContextMenuCallback, userActionPrefix);
    }

    @Override
    protected boolean shouldShowItem(@ContextMenuItemId int itemId, Delegate delegate) {
        if (itemId == ContextMenuItemId.OPEN_IN_NEW_TAB_IN_GROUP
                && !BraveTabUiFeatureUtilities.isTabGroupsEnabled()) {
            return false;
        }
        return super.shouldShowItem(itemId, delegate);
    }
}
