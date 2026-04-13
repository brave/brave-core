/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp;

import android.app.Activity;

import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.build.annotations.Initializer;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.feed.FeedSurfaceScrollDelegate;
import org.chromium.chrome.browser.lifecycle.ActivityLifecycleDispatcher;
import org.chromium.chrome.browser.magic_stack.ModuleRegistry;
import org.chromium.chrome.browser.preloading.AndroidPrerenderManager;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.suggestions.mostvisited.MostVisitedSites;
import org.chromium.chrome.browser.suggestions.tile.Tile;
import org.chromium.chrome.browser.suggestions.tile.TileGroup;
import org.chromium.chrome.browser.suggestions.tile.TileSource;
import org.chromium.chrome.browser.suggestions.tile.tile_edit_dialog.CustomTileEditCoordinator;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.tasks.HomeSurfaceTracker;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager;
import org.chromium.chrome.browser.ui.native_page.TouchEnabledDelegate;
import org.chromium.components.browser_ui.bottomsheet.BottomSheetController;
import org.chromium.components.browser_ui.widget.displaystyle.UiConfig;
import org.chromium.misc_metrics.mojom.MiscAndroidMetrics;
import org.chromium.ui.base.ActivityResultTracker;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.modaldialog.ModalDialogManager;
import org.chromium.url.GURL;

import java.util.List;
import java.util.function.Supplier;

@NullMarked
public class BraveNewTabPageCoordinator extends NewTabPageCoordinator {
    private static final String TAG = "BraveNewTabPageCoordinator";

    private final Activity mActivity;

    private final BraveNewTabPageLayout mBraveNewTabPageLayout;
    private final NewTabPageManager mNewTabPageManager;

    private final Profile mProfile;
    private final WindowAndroid mWindowAndroid;

    public BraveNewTabPageCoordinator(
            NewTabPageManager manager,
            Activity activity,
            NewTabPageLayout newTabPageLayout,
            Tab tab,
            TabModelSelector tabModelSelector,
            OneshotSupplier<ModuleRegistry> moduleRegistrySupplier,
            Profile profile,
            WindowAndroid windowAndroid,
            ActivityResultTracker activityResultTracker,
            BottomSheetController bottomSheetController,
            ModalDialogManager modalDialogManager,
            SnackbarManager snackbarManager,
            boolean isTablet,
            Supplier<Integer> tabStripHeightSupplier,
            @Nullable HomeSurfaceTracker homeSurfaceTracker) {
        super(
                manager,
                activity,
                newTabPageLayout,
                tab,
                tabModelSelector,
                moduleRegistrySupplier,
                profile,
                windowAndroid,
                activityResultTracker,
                bottomSheetController,
                modalDialogManager,
                snackbarManager,
                isTablet,
                tabStripHeightSupplier,
                homeSurfaceTracker);

        mNewTabPageManager = manager;

        assert (activity instanceof BraveActivity);
        mActivity = activity;

        assert (newTabPageLayout instanceof BraveNewTabPageLayout);
        mBraveNewTabPageLayout = (BraveNewTabPageLayout) newTabPageLayout;

        mProfile = profile;
        mWindowAndroid = windowAndroid;
    }

    @Override
    @Initializer
    public void initialize(
            TileGroup.Delegate tileGroupDelegate,
            boolean searchProviderHasLogo,
            boolean searchProviderIsGoogle,
            FeedSurfaceScrollDelegate scrollDelegate,
            TouchEnabledDelegate touchEnabledDelegate,
            UiConfig uiConfig,
            ActivityLifecycleDispatcher lifecycleDispatcher,
            Supplier<GURL> composeplateUrlSupplier) {
        super.initialize(
                new BraveTileGroupDelegate(tileGroupDelegate, mActivity),
                searchProviderHasLogo,
                searchProviderIsGoogle,
                scrollDelegate,
                touchEnabledDelegate,
                uiConfig,
                lifecycleDispatcher,
                composeplateUrlSupplier);

        mBraveNewTabPageLayout.initialize(mNewTabPageManager, mActivity, mProfile, mWindowAndroid);
    }

    private static class BraveTileGroupDelegate implements TileGroup.Delegate {
        private final TileGroup.Delegate mDelegate;
        private final Activity mActivity;

        BraveTileGroupDelegate(TileGroup.Delegate delegate, Activity activity) {
            mDelegate = delegate;
            mActivity = activity;
        }

        @Override
        public void openMostVisitedItem(int windowDisposition, Tile tile) {
            if (mActivity instanceof BraveActivity braveActivity) {
                MiscAndroidMetrics metrics = braveActivity.getMiscAndroidMetrics();
                if (metrics != null) {
                    metrics.recordTopSiteNavigation(tile.getSource() == TileSource.CUSTOM_LINKS);
                }
            }
            mDelegate.openMostVisitedItem(windowDisposition, tile);
        }

        @Override
        public void openMostVisitedItemInGroup(int windowDisposition, Tile tile) {
            mDelegate.openMostVisitedItemInGroup(windowDisposition, tile);
        }

        @Override
        public void removeMostVisitedItem(Tile tile) {
            mDelegate.removeMostVisitedItem(tile);
        }

        @Override
        public void setMostVisitedSitesObserver(
                MostVisitedSites.Observer observer, int maxResults) {
            mDelegate.setMostVisitedSitesObserver(observer, maxResults);
        }

        @Override
        public void onLoadingComplete(List<Tile> tiles) {
            mDelegate.onLoadingComplete(tiles);
        }

        @Override
        public void initAndroidPrerenderManager(AndroidPrerenderManager androidPrerenderManager) {
            mDelegate.initAndroidPrerenderManager(androidPrerenderManager);
        }

        @Override
        public CustomTileEditCoordinator createCustomTileEditCoordinator(
                @Nullable Tile originalTile) {
            return mDelegate.createCustomTileEditCoordinator(originalTile);
        }

        @Override
        public void showTileUnpinSnackbar(Runnable undoHandler) {
            mDelegate.showTileUnpinSnackbar(undoHandler);
        }

        @Override
        public double getSuggestionScore(GURL url) {
            return mDelegate.getSuggestionScore(url);
        }

        @Override
        public void destroy() {
            mDelegate.destroy();
        }

        @Override
        public boolean addCustomLink(String name, @Nullable GURL url, @Nullable Integer pos) {
            return mDelegate.addCustomLink(name, url, pos);
        }

        @Override
        public boolean assignCustomLink(GURL keyUrl, String name, @Nullable GURL url) {
            return mDelegate.assignCustomLink(keyUrl, name, url);
        }

        @Override
        public boolean deleteCustomLink(GURL keyUrl) {
            return mDelegate.deleteCustomLink(keyUrl);
        }

        @Override
        public boolean hasCustomLink(GURL keyUrl) {
            return mDelegate.hasCustomLink(keyUrl);
        }

        @Override
        public boolean reorderCustomLink(GURL keyUrl, int newPos) {
            return mDelegate.reorderCustomLink(keyUrl, newPos);
        }
    }
}
