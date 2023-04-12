/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.bottom;

import org.chromium.ui.modelutil.PropertyModel;

/**
 * All of the state for the bottom toolbar, updated by the {@link
 * BrowsingModeBottomToolbarCoordinator}.
 */
public class BrowsingModeBottomToolbarModel extends PropertyModel {
    /** Primary color of bottom toolbar. */
    static final WritableIntPropertyKey PRIMARY_COLOR = new WritableIntPropertyKey();

    /** Whether the browsing mode bottom toolbar is visible */
    static final WritableBooleanPropertyKey IS_VISIBLE = new WritableBooleanPropertyKey();

    /** Default constructor. */
    BrowsingModeBottomToolbarModel() {
        super(IS_VISIBLE, PRIMARY_COLOR);
        set(IS_VISIBLE, true);
    }
}
