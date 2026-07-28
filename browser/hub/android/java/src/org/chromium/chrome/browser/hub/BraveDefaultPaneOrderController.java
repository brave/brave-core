/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.hub;

import com.google.common.collect.ImmutableSet;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;

/** Brave's {@link PaneOrderController} that can hide the Tab Groups pane. */
@NullMarked
public class BraveDefaultPaneOrderController extends DefaultPaneOrderController {
    @Override
    public ImmutableSet<Integer> getPaneOrder() {
        ImmutableSet<Integer> paneOrder = super.getPaneOrder();
        // Drop the Tab Groups pane from the Hub (tab switcher overview) while the
        // "Enable tab groups" master switch is off, so its overview toggle/icon is
        // gone and users cannot open the pane to create groups. Existing groups and
        // group sync/restore are unaffected.
        if (ChromeSharedPreferences.getInstance()
                .readBoolean(BravePreferenceKeys.BRAVE_TAB_GROUPS_FEATURE_ENABLED, true)) {
            return paneOrder;
        }
        ImmutableSet.Builder<Integer> builder = ImmutableSet.builder();
        for (Integer paneId : paneOrder) {
            if (paneId != PaneId.TAB_GROUPS) {
                builder.add(paneId);
            }
        }
        return builder.build();
    }
}
