/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.compositor.layouts;

import android.content.Context;
import android.view.ViewGroup;

import org.chromium.base.supplier.Supplier;
import org.chromium.chrome.browser.browser_controls.BrowserControlsStateProvider;
import org.chromium.chrome.browser.layouts.LayoutManager;
import org.chromium.chrome.browser.theme.TopUiThemeColorProvider;

/** Layout defining the animation and positioning of the tabs during the edge swipe effect. */
public class BraveToolbarSwipeLayout extends ToolbarSwipeLayout {
    /**
     * Whether or not to move toolbar with tab contents. Will be deleted in bytecode, value from the
     * parent class will be used instead.
     */
    @SuppressWarnings("UnusedVariable")
    private boolean mMoveToolbar;

    public BraveToolbarSwipeLayout(
            Context context,
            LayoutUpdateHost updateHost,
            LayoutRenderHost renderHost,
            BrowserControlsStateProvider browserControlsStateProvider,
            LayoutManager layoutManager,
            TopUiThemeColorProvider topUiColorProvider,
            Supplier<Integer> bottomControlsOffsetSupplier,
            ViewGroup contentContainer) {
        super(
                context,
                updateHost,
                renderHost,
                browserControlsStateProvider,
                layoutManager,
                topUiColorProvider,
                bottomControlsOffsetSupplier,
                contentContainer);

        // To postpone toolbar transition animation to the end of the swipe.
        mMoveToolbar = false;
    }
}
