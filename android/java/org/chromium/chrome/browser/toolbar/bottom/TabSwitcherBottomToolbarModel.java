// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

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
