/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabWebContentsUserData;
import org.chromium.content_public.browser.WebContents;
import org.chromium.ui.OverscrollAction;
import org.chromium.ui.OverscrollRefreshHandler;
import org.chromium.ui.base.BackGestureEventSwipeEdge;

@NullMarked
public class BraveSwipeRefreshHandler extends TabWebContentsUserData
        implements OverscrollRefreshHandler {
    private static final Class<BraveSwipeRefreshHandler> USER_DATA_KEY =
            BraveSwipeRefreshHandler.class;

    private final SwipeRefreshHandler mDelegate;
    private @OverscrollAction int mSwipeType;
    private boolean mIgnorePullToRefresh;

    public static BraveSwipeRefreshHandler from(Tab tab) {
        BraveSwipeRefreshHandler handler = get(tab);
        if (handler == null) {
            handler =
                    tab.getUserDataHost()
                            .setUserData(USER_DATA_KEY, new BraveSwipeRefreshHandler(tab));
        }
        return handler;
    }

    public static @Nullable BraveSwipeRefreshHandler get(Tab tab) {
        return tab.getUserDataHost().getUserData(USER_DATA_KEY);
    }

    private BraveSwipeRefreshHandler(Tab tab) {
        super(tab);

        SwipeRefreshHandler delegate = SwipeRefreshHandler.get(tab);
        assert delegate != null;
        mDelegate = delegate;
    }

    public void setIgnorePullToRefresh(boolean ignorePullToRefresh) {
        mIgnorePullToRefresh = ignorePullToRefresh;
    }

    @Override
    public void initWebContents(WebContents webContents) {
        webContents.setOverscrollRefreshHandler(this);
    }

    @Override
    public void cleanupWebContents(WebContents webContents) {}

    @Override
    public boolean start(
            @OverscrollAction int type, @BackGestureEventSwipeEdge int initiatingEdge) {
        mSwipeType = type;
        if (!mIgnorePullToRefresh || mSwipeType != OverscrollAction.PULL_TO_REFRESH) {
            return mDelegate.start(type, initiatingEdge);
        }
        return true;
    }

    @Override
    public void pull(float xDelta, float yDelta) {
        if (!mIgnorePullToRefresh || mSwipeType != OverscrollAction.PULL_TO_REFRESH) {
            mDelegate.pull(xDelta, yDelta);
        }
    }

    @Override
    public void release(boolean allowRefresh) {
        if (!mIgnorePullToRefresh || mSwipeType != OverscrollAction.PULL_TO_REFRESH) {
            mDelegate.release(allowRefresh);
        }
    }

    @Override
    public void reset() {
        mDelegate.reset();
    }

    @Override
    public void setEnabled(boolean enabled) {
        mDelegate.setEnabled(enabled);
    }
}
