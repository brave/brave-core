/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.suggestions.tile;

import android.content.res.Resources;
import android.view.ViewStub;

import androidx.annotation.Nullable;

import org.chromium.chrome.browser.widget.quickactionsearchandbookmark.QuickActionSearchAndBookmarkWidgetProvider;
import org.chromium.components.browser_ui.widget.displaystyle.UiConfig;
import org.chromium.ui.modelutil.PropertyModel;

public class BraveMostVisitedTilesMediator extends MostVisitedTilesMediator {
    private TileGroup mTileGroup;

    public BraveMostVisitedTilesMediator(
            Resources resources,
            UiConfig uiConfig,
            MostVisitedTilesLayout mvTilesLayout,
            ViewStub noMvPlaceholderStub,
            TileRenderer renderer,
            PropertyModel propertyModel,
            boolean isTablet,
            @Nullable Runnable snapshotTileGridChangedRunnable,
            @Nullable Runnable tileCountChangedRunnable) {
        super(
                resources,
                uiConfig,
                mvTilesLayout,
                noMvPlaceholderStub,
                renderer,
                propertyModel,
                isTablet,
                snapshotTileGridChangedRunnable,
                tileCountChangedRunnable);
    }

    protected void updateTilePlaceholderVisibility() {
        // This function is kept empty to avoid placeholder implementation
    }

    @Override
    public void onTileDataChanged() {
        super.onTileDataChanged();
        QuickActionSearchAndBookmarkWidgetProvider.DataManager.parseTilesAndWriteWidgetTiles(
                mTileGroup.getTileSections().get(TileSectionType.PERSONALIZED));
    }

    @Override
    public void onTileIconChanged(Tile tile) {
        super.onTileIconChanged(tile);
        QuickActionSearchAndBookmarkWidgetProvider.updateTileIcon(tile);
    }
}
