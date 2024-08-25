/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.undo_tab_close_snackbar;

import android.content.Context;

import androidx.annotation.Nullable;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.supplier.Supplier;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager.SnackbarManageable;

import java.util.List;

public class BraveUndoBarController extends UndoBarController {
    public BraveUndoBarController(
            Context context,
            TabModelSelector selector,
            SnackbarManageable snackbarManageable,
            @Nullable Supplier<Boolean> dialogVisibilitySupplier) {
        super(context, selector, snackbarManageable, dialogVisibilitySupplier);
    }

    @Override
    protected void showUndoBar(List<Tab> closedTabs, boolean isAllTabs) {
        boolean should_show_undo_bar =
                ChromeSharedPreferences.getInstance()
                        .readBoolean(BravePreferenceKeys.SHOW_UNDO_ON_TAB_CLOSED, true);
        if (!should_show_undo_bar) {
            return;
        }
        super.showUndoBar(closedTabs, isAllTabs);
    }
}
