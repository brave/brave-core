/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.util.TabUtils;
import org.chromium.ui.OverscrollAction;
import org.chromium.ui.base.BackGestureEventSwipeEdge;
import org.chromium.ui.base.PageTransition;
import org.chromium.url.GURL;

/** Override of upstream's class to deny refresh by pulldown for Brave Leo while it is in a panel */
public class BraveSwipeRefreshHandler extends SwipeRefreshHandler {
    public @OverscrollAction int mSwipeType;
    public Tab mTab;

    // There is a direct patch for SwipeRefreshHandler to make it's ctor public
    public BraveSwipeRefreshHandler(Tab tab) {
        super(tab);
    }

    @Override
    public boolean start(
            @OverscrollAction int type, @BackGestureEventSwipeEdge int initiatingEdge) {
        GURL url = mTab.getUrl();
        if (url.getScheme().equals("chrome-untrusted")
                && url.getHost().equals("chat")
                && TabUtils.getTransition(mTab) == PageTransition.FROM_API) {
            mSwipeType = OverscrollAction.NONE;
            return false;
        }

        return super.start(type, initiatingEdge);
    }
}
