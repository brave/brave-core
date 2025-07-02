/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.undo_tab_close_snackbar;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.BraveReflectionUtil;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager;
import org.chromium.chrome.browser.undo_tab_close_snackbar.UndoBarController.UndoActionData;

import java.util.List;

public class BraveUndoBarControllerBase implements SnackbarManager.SnackbarController {
    public void showUndoBar(
            List<Tab> closedTabs, List<String> savedTabGroupSyncIds, boolean isAllTabs) {
        boolean showUndoBar =
                ChromeSharedPreferences.getInstance()
                        .readBoolean(BravePreferenceKeys.SHOW_UNDO_WHEN_TABS_CLOSED, true);
        if (!showUndoBar) {
            if (closedTabs.isEmpty()) {
                return;
            }
            UndoActionData undoActionData = new UndoActionData(closedTabs, savedTabGroupSyncIds);
            onDismissNoAction(undoActionData);
            return;
        }

        BraveReflectionUtil.invokeMethod(
                UndoBarController.class,
                this,
                "showUndoBar",
                List.class,
                closedTabs,
                List.class,
                savedTabGroupSyncIds,
                boolean.class,
                isAllTabs);
    }
}
