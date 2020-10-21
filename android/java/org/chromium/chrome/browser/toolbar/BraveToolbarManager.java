/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar;

import android.content.res.Configuration;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewStub;

import androidx.annotation.Nullable;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.Callback;
import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.base.supplier.Supplier;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ActivityTabProvider;
import org.chromium.chrome.browser.app.ChromeActivity;
import org.chromium.chrome.browser.bookmarks.BookmarkBridge;
import org.chromium.chrome.browser.browser_controls.BrowserControlsSizer;
import org.chromium.chrome.browser.compositor.Invalidator;
import org.chromium.chrome.browser.compositor.layouts.LayoutManager;
import org.chromium.chrome.browser.compositor.layouts.OverviewModeBehavior;
import org.chromium.chrome.browser.findinpage.FindToolbarManager;
import org.chromium.chrome.browser.fullscreen.FullscreenManager;
import org.chromium.chrome.browser.identity_disc.IdentityDiscController;
import org.chromium.chrome.browser.omnibox.LocationBar;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.share.ShareDelegate;
import org.chromium.chrome.browser.tab.SadTab;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.toolbar.bottom.BottomControlsCoordinator;
import org.chromium.chrome.browser.toolbar.bottom.BottomTabSwitcherActionMenuCoordinator;
import org.chromium.chrome.browser.toolbar.bottom.BottomToolbarConfiguration;
import org.chromium.chrome.browser.toolbar.bottom.BottomToolbarVariationManager;
import org.chromium.chrome.browser.toolbar.bottom.BraveBottomControlsCoordinator;
import org.chromium.chrome.browser.toolbar.menu_button.BraveMenuButtonCoordinator;
import org.chromium.chrome.browser.toolbar.menu_button.MenuButtonCoordinator;
import org.chromium.chrome.browser.toolbar.top.ActionModeController;
import org.chromium.chrome.browser.toolbar.top.BraveTopToolbarCoordinator;
import org.chromium.chrome.browser.toolbar.top.ToolbarActionModeCallback;
import org.chromium.chrome.browser.toolbar.top.ToolbarControlContainer;
import org.chromium.chrome.browser.toolbar.top.TopToolbarCoordinator;
import org.chromium.chrome.browser.ui.TabObscuringHandler;
import org.chromium.chrome.browser.ui.appmenu.AppMenuCoordinator;
import org.chromium.chrome.features.start_surface.StartSurface;
import org.chromium.components.browser_ui.widget.scrim.ScrimCoordinator;

import java.util.List;

public class BraveToolbarManager extends ToolbarManager {
    private BottomControlsCoordinator mBottomControlsCoordinator;
    private BrowserControlsSizer mBrowserControlsSizer;
    private FullscreenManager mFullscreenManager;
    private ActivityTabProvider mActivityTabProvider;
    private AppThemeColorProvider mAppThemeColorProvider;
    private ObservableSupplier<ShareDelegate> mShareDelegateSupplier;
    private ScrimCoordinator mScrimCoordinator;
    private ChromeActivity mActivity;
    private Supplier<Boolean> mShowStartSurfaceSupplier;
    private OneshotSupplier<OverviewModeBehavior> mOverviewModeBehaviorSupplier;
    private MenuButtonCoordinator mMenuButtonCoordinator;
    private ToolbarTabControllerImpl mToolbarTabController;
    private LocationBar mLocationBar;
    private ActionModeController mActionModeController;
    private LocationBarModel mLocationBarModel;
    private TopToolbarCoordinator mToolbar;
    private ObservableSupplier<BookmarkBridge> mBookmarkBridgeSupplier;

    private boolean mIsBottomToolbarVisible;
    private View mRootBottomView;

    public BraveToolbarManager(ChromeActivity activity, BrowserControlsSizer controlsSizer,
            FullscreenManager fullscreenManager, ToolbarControlContainer controlContainer,
            Invalidator invalidator, Callback<Boolean> urlFocusChangedCallback,
            ThemeColorProvider themeColorProvider, TabObscuringHandler tabObscuringHandler,
            ObservableSupplier<ShareDelegate> shareDelegateSupplier,
            IdentityDiscController identityDiscController,
            List<ButtonDataProvider> buttonDataProviders, ActivityTabProvider tabProvider,
            ScrimCoordinator scrimCoordinator, ToolbarActionModeCallback toolbarActionModeCallback,
            FindToolbarManager findToolbarManager, ObservableSupplier<Profile> profileSupplier,
            ObservableSupplier<BookmarkBridge> bookmarkBridgeSupplier,
            @Nullable Supplier<Boolean> canAnimateNativeBrowserControls,
            OneshotSupplier<OverviewModeBehavior> overviewModeBehaviorSupplier,
            OneshotSupplier<AppMenuCoordinator> appMenuCoordinatorSupplier,
            boolean shouldShowUpdateBadge,
            ObservableSupplier<TabModelSelector> tabModelSelectorSupplier,
            OneshotSupplier<StartSurface> startSurfaceSupplier) {
        super(activity, controlsSizer, fullscreenManager, controlContainer, invalidator,
                urlFocusChangedCallback, themeColorProvider, tabObscuringHandler,
                shareDelegateSupplier, identityDiscController, buttonDataProviders, tabProvider,
                scrimCoordinator, toolbarActionModeCallback, findToolbarManager, profileSupplier,
                bookmarkBridgeSupplier, canAnimateNativeBrowserControls,
                overviewModeBehaviorSupplier, appMenuCoordinatorSupplier, shouldShowUpdateBadge,
                tabModelSelectorSupplier, startSurfaceSupplier);
    }

    @Override
    public void enableBottomControls() {
        ViewStub viewStub = mActivity.findViewById(R.id.bottom_controls_stub);
        if (!BottomToolbarConfiguration.isBottomToolbarEnabled() || viewStub == null) {
            super.enableBottomControls();
            return;
        }
        viewStub.setOnInflateListener((stub, inflated) -> { mRootBottomView = inflated; });
        mBottomControlsCoordinator = new BraveBottomControlsCoordinator(
                BottomTabSwitcherActionMenuCoordinator.createOnLongClickListener(
                        id -> mActivity.onOptionsItemSelected(id, null)),
                mBrowserControlsSizer, mFullscreenManager, viewStub, mActivityTabProvider,
                mAppThemeColorProvider, mShareDelegateSupplier,
                mMenuButtonCoordinator.getMenuButtonHelperSupplier(), mShowStartSurfaceSupplier,
                mToolbarTabController::openHomepage,
                (reason)
                        -> setUrlBarFocus(true, reason),
                mOverviewModeBehaviorSupplier, mScrimCoordinator);
        ((BraveBottomControlsCoordinator) mBottomControlsCoordinator).setRootView(mRootBottomView);
        boolean isBottomToolbarVisible = BottomToolbarConfiguration.isBottomToolbarEnabled()
                && mActivity.getResources().getConfiguration().orientation
                        != Configuration.ORIENTATION_LANDSCAPE;
        setBottomToolbarVisible(isBottomToolbarVisible);
    }

    @Override
    public void initializeWithNative(LayoutManager layoutManager,
            OnClickListener tabSwitcherClickHandler, OnClickListener newTabClickHandler,
            OnClickListener bookmarkClickHandler, OnClickListener customTabsBackClickHandler,
            Supplier<Boolean> showStartSurfaceSupplier) {
        super.initializeWithNative(layoutManager, tabSwitcherClickHandler, newTabClickHandler,
                bookmarkClickHandler, customTabsBackClickHandler, showStartSurfaceSupplier);

        if (mBottomControlsCoordinator != null) {
            ApiCompatibilityUtils.setAccessibilityTraversalBefore(
                    mLocationBar.getContainerView(), R.id.bottom_toolbar);
        }
    }

    @Override
    public @Nullable View getMenuButtonView() {
        if (mMenuButtonCoordinator.getMenuButton() == null) {
            // Return fake view instead of null to avoid NullPointerException as some code within
            // Chromium doesn't check for null.
            return new View(mActivity);
        }
        return super.getMenuButtonView();
    }

    protected void onOrientationChange(int newOrientation) {
        if (mActionModeController != null) mActionModeController.showControlsOnOrientationChange();

        if (mBottomControlsCoordinator != null
                && BottomToolbarConfiguration.isBottomToolbarEnabled()) {
            boolean isBottomToolbarVisible = newOrientation != Configuration.ORIENTATION_LANDSCAPE;
            setBottomToolbarVisible(isBottomToolbarVisible);
        }
    }

    protected void updateButtonStatus() {
        Tab currentTab = mLocationBarModel.getTab();
        boolean tabCrashed = currentTab != null && SadTab.isShowing(currentTab);

        mToolbar.updateButtonVisibility();
        mToolbar.updateBackButtonVisibility(currentTab != null && currentTab.canGoBack());
        mToolbar.updateForwardButtonVisibility(currentTab != null && currentTab.canGoForward());
        updateReloadState(tabCrashed);
        updateBookmarkButtonStatus();

        if (mToolbar.getMenuButtonWrapper() != null && !isBottomToolbarVisible()) {
            mToolbar.getMenuButtonWrapper().setVisibility(View.VISIBLE);
        }
    }

    protected void updateBookmarkButtonStatus() {
        Tab currentTab = mLocationBarModel.getTab();
        BookmarkBridge bridge = mBookmarkBridgeSupplier.get();
        boolean isBookmarked =
                currentTab != null && bridge != null && bridge.hasBookmarkIdForTab(currentTab);
        boolean editingAllowed =
                currentTab == null || bridge == null || bridge.isEditBookmarksEnabled();
        mToolbar.updateBookmarkButton(isBookmarked, editingAllowed);

        if (mBottomControlsCoordinator instanceof BraveBottomControlsCoordinator) {
            ((BraveBottomControlsCoordinator) mBottomControlsCoordinator)
                    .updateBookmarkButton(isBookmarked, editingAllowed);
        }
    }

    protected void updateReloadState(boolean tabCrashed) {
        assert (false);
    }

    private void setBottomToolbarVisible(boolean visible) {
        mIsBottomToolbarVisible = visible;
        Boolean isMenuFromBottom =
                mIsBottomToolbarVisible && BottomToolbarConfiguration.isBottomToolbarEnabled();
        BraveMenuButtonCoordinator.setMenuFromBottom(isMenuFromBottom);
        if (mToolbar instanceof BraveTopToolbarCoordinator) {
            ((BraveTopToolbarCoordinator) mToolbar).onBottomToolbarVisibilityChanged(visible);
        }
        mBottomControlsCoordinator.setBottomControlsVisible(visible);
    }

    public boolean isBottomToolbarVisible() {
        return mIsBottomToolbarVisible;
    }
}
