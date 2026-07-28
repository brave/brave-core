/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tabmodel;

import org.chromium.base.Token;
import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.app.tabwindow.TabWindowManagerSingleton;

import java.util.ArrayList;
import java.util.List;

/** Brave-specific tab group helpers. */
@NullMarked
public class BraveTabGroupHelper {
    private BraveTabGroupHelper() {}

    /**
     * Ungroups every tab group of every loaded window, both regular and incognito. The tabs that
     * were in those groups stay open as regular ungrouped tabs. Called when the user turns the
     * "Enable tab groups" setting off, after they confirmed that existing groups get removed.
     */
    public static void ungroupAllTabGroups() {
        for (TabModelSelector tabModelSelector :
                TabWindowManagerSingleton.getInstance().getAllTabModelSelectors()) {
            for (TabModel tabModel : tabModelSelector.getModels()) {
                ungroupAllTabGroups(tabModel);
            }
        }
    }

    private static void ungroupAllTabGroups(TabModel tabModel) {
        // Ungrouping mutates the model, so iterate over a copy of the group IDs.
        List<Token> tabGroupIds = new ArrayList<>(tabModel.getAllTabGroupIds());
        for (Token tabGroupId : tabGroupIds) {
            tabModel.getTabUngrouper()
                    .ungroupTabGroup(
                            tabGroupId,
                            /* trailing= */ true,
                            // The user already confirmed this in settings, so don't ask again per
                            // group.
                            /* allowDialog= */ false);
        }
    }
}
