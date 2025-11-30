/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.ui.OverscrollAction;
import org.chromium.ui.base.BackGestureEventSwipeEdge;

@NullMarked
public class BraveSwipeRefreshHandler extends SwipeRefreshHandler {
    public boolean mIgnorePullToRefresh;

    BraveSwipeRefreshHandler(Tab tab, SwipeRefreshLayoutCreator swipeRefreshLayoutCreator) {
        super(tab, swipeRefreshLayoutCreator);
    }

    @Override
    public boolean start(
            @OverscrollAction int type, @BackGestureEventSwipeEdge int initiatingEdge) {
        if (mIgnorePullToRefresh && type == OverscrollAction.PULL_TO_REFRESH) {
            return true;
        }

        return super.start(type, initiatingEdge);
    }
}
