/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ui.bottombar;

import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.ui.actions.ActionId;
import org.chromium.chrome.browser.ui.actions.ActionRegistry;
import org.chromium.ui.modelutil.PropertyModel;

import java.util.List;

/** Brave's extension of {@link BottomBarButtonManager}. */
@NullMarked
public class BraveBottomBarButtonManager extends BottomBarButtonManager {
    public BraveBottomBarButtonManager(
            List<ActionConfig> configs,
            ActionRegistry actionRegistry,
            PropertyModel bottomBarModel,
            int centerActionId) {
        super(configs, actionRegistry, bottomBarModel, centerActionId(configs, centerActionId));
    }

    /**
     * Makes Brave's search accelerator the emphasized centre button, in place of upstream's new tab
     * button.
     *
     * <p>Falls back to upstream's choice where Brave's button set is not in use - the bottom bar is
     * also built straight from {@link BottomBarCoordinator} in its own unit tests.
     */
    private static int centerActionId(List<ActionConfig> configs, int upstreamCenterActionId) {
        for (ActionConfig config : configs) {
            if (config.actionId == ActionId.BRAVE_SEARCH) return ActionId.BRAVE_SEARCH;
        }
        return upstreamCenterActionId;
    }
}
