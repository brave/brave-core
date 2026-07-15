/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.suggestions.tile;

import android.view.View;

import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.native_page.BraveNtpDelegate;
import org.chromium.chrome.browser.native_page.ContextMenuManager;
import org.chromium.chrome.browser.ntp.NtpUtil;

/**
 * Brave extension of {@link TileInteractionDelegateImpl} that also implements {@link
 * BraveNtpDelegate}, adding "Add site", "Show frequently visited", "Show shortcuts", and "Hide
 * widget" items to the tile long-press context menu.
 */
@NullMarked
class BraveTileInteractionDelegateImpl extends TileInteractionDelegateImpl
        implements BraveNtpDelegate {
    private final TileGroup.CustomTileModificationDelegate mBraveCustomTileModificationDelegate;

    BraveTileInteractionDelegateImpl(
            ContextMenuManager contextMenuManager,
            TileGroup.Delegate tileGroupDelegate,
            TileDragDelegate tileDragDelegate,
            TileGroup.CustomTileModificationDelegate customTileModificationDelegate,
            Tile tile,
            View view) {
        super(
                contextMenuManager,
                tileGroupDelegate,
                tileDragDelegate,
                customTileModificationDelegate,
                tile,
                view);
        mBraveCustomTileModificationDelegate = customTileModificationDelegate;
    }

    @Override
    public boolean isBraveItemSupported(int menuItemId) {
        return menuItemId == BRAVE_ADD_SITE
                || menuItemId == BRAVE_SHOW_FREQUENT
                || menuItemId == BRAVE_SHOW_SHORTCUTS
                || menuItemId == BRAVE_HIDE_WIDGET;
    }

    @Override
    public void braveAddSite() {
        mBraveCustomTileModificationDelegate.add();
    }

    @Override
    public void braveShowFrequent() {
        NtpUtil.setTopSitesDisplayMode(NtpUtil.TOP_SITES_MODE_FREQUENT);
    }

    @Override
    public void braveShowShortcuts() {
        NtpUtil.setTopSitesDisplayMode(NtpUtil.TOP_SITES_MODE_SHORTCUTS);
    }

    @Override
    public void braveHideWidget() {
        NtpUtil.setDisplayTopSites(false);
    }

    @Override
    public int getBraveTopSitesDisplayMode() {
        return NtpUtil.getTopSitesDisplayMode();
    }

    @Override
    public int getSelectedModeEndIconRes() {
        return R.drawable.ic_check_circle_filled;
    }
}
