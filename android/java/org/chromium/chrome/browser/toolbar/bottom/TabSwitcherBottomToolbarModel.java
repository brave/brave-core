/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.bottom;

import org.chromium.ui.modelutil.PropertyModel;

/**
 * All of the state for the tab switcher bottom toolbar, updated by the
 * {@link TabSwitcherBottomToolbarCoordinator}.
 */
public class TabSwitcherBottomToolbarModel extends PropertyModel {
    /** Primary color of tab switcher bottom toolbar. */
    public static final WritableIntPropertyKey PRIMARY_COLOR = new WritableIntPropertyKey();

    /** Whether the tab switcher bottom toolbar is visible */
    public static final WritableBooleanPropertyKey IS_VISIBLE = new WritableBooleanPropertyKey();

    /** Whether the tab switcher bottom toolbar shows on top of the screen. */
    public static final WritableBooleanPropertyKey SHOW_ON_TOP = new WritableBooleanPropertyKey();

    /** Default constructor. */
    public TabSwitcherBottomToolbarModel() {
        super(PRIMARY_COLOR, IS_VISIBLE, SHOW_ON_TOP);
    }
}
