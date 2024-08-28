/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.bottom;

import android.app.Activity;
import android.view.View.OnClickListener;
import android.view.View.OnLongClickListener;
import android.view.ViewGroup;

import androidx.annotation.Nullable;

import org.chromium.base.Callback;
import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.ObservableSupplierImpl;
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.base.supplier.Supplier;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ActivityTabProvider;
import org.chromium.chrome.browser.bookmarks.BookmarkModel;
import org.chromium.chrome.browser.browser_controls.BottomControlsStacker;
import org.chromium.chrome.browser.browser_controls.BrowserStateBrowserControlsVisibilityDelegate;
import org.chromium.chrome.browser.compositor.layouts.LayoutManagerImpl;
import org.chromium.chrome.browser.fullscreen.FullscreenManager;
import org.chromium.chrome.browser.layouts.LayoutManager;
import org.chromium.chrome.browser.layouts.LayoutStateProvider;
import org.chromium.chrome.browser.tab.TabObscuringHandler;
import org.chromium.chrome.browser.tabmodel.IncognitoStateProvider;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.theme.ThemeColorProvider;
import org.chromium.chrome.browser.toolbar.LocationBarModel;
import org.chromium.chrome.browser.ui.appmenu.AppMenuButtonHelper;
import org.chromium.chrome.browser.ui.edge_to_edge.EdgeToEdgeController;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.resources.ResourceManager;

public class BraveBottomControlsCoordinator extends BottomControlsCoordinator {
    // To delete in bytecode, members from parent class will be used instead.
    private BottomControlsMediator mMediator;

    // Own members.
    private @Nullable BottomToolbarCoordinator mBottomToolbarCoordinator;
    private OnLongClickListener mTabSwitcherLongclickListener;
    private ActivityTabProvider mTabProvider;
    private ThemeColorProvider mThemeColorProvider;
    private ObservableSupplier<AppMenuButtonHelper> mMenuButtonHelperSupplier;
    private Runnable mOpenHomepageAction;
    private Callback<Integer> mSetUrlBarFocusAction;
    private OneshotSupplier<LayoutStateProvider> mLayoutStateProviderSupplier;
    private ScrollingBottomViewResourceFrameLayout mRoot;
    private ObservableSupplier<BookmarkModel> mBookmarkModelSupplier;
    private LocationBarModel mLocationBarModel;

    public BraveBottomControlsCoordinator(
            OneshotSupplier<LayoutStateProvider> layoutStateProviderSupplier,
            OnLongClickListener tabSwitcherLongclickListener,
            ActivityTabProvider tabProvider,
            Runnable openHomepageAction,
            Callback<Integer> setUrlBarFocusAction,
            ObservableSupplier<AppMenuButtonHelper> menuButtonHelperSupplier,
            ThemeColorProvider themeColorProvider,
            ObservableSupplier<BookmarkModel> bookmarkModelSupplier,
            LocationBarModel locationBarModel,
            /* Below are parameters from BottomControlsCoordinator */
            Activity activity,
            WindowAndroid windowAndroid,
            LayoutManager layoutManager,
            ResourceManager resourceManager,
            BottomControlsStacker controlsStacker,
            BrowserStateBrowserControlsVisibilityDelegate browserControlsVisibilityDelegate,
            FullscreenManager fullscreenManager,
            ObservableSupplier<EdgeToEdgeController> edgeToEdgeControllerSupplier,
            ScrollingBottomViewResourceFrameLayout root,
            OneshotSupplier<BottomControlsContentDelegate> contentDelegateSupplier,
            TabObscuringHandler tabObscuringHandler,
            ObservableSupplier<Boolean> overlayPanelVisibilitySupplier,
            ObservableSupplier<Integer> constraintsSupplier,
            Supplier<Boolean> readAloudRestoringSupplier) {
        super(
                activity,
                windowAndroid,
                layoutManager,
                resourceManager,
                controlsStacker,
                browserControlsVisibilityDelegate,
                fullscreenManager,
                edgeToEdgeControllerSupplier,
                root,
                contentDelegateSupplier,
                tabObscuringHandler,
                overlayPanelVisibilitySupplier,
                constraintsSupplier,
                readAloudRestoringSupplier);

        mTabSwitcherLongclickListener = tabSwitcherLongclickListener;
        mTabProvider = tabProvider;
        mThemeColorProvider = themeColorProvider;
        mOpenHomepageAction = openHomepageAction;
        mSetUrlBarFocusAction = setUrlBarFocusAction;
        mLayoutStateProviderSupplier = layoutStateProviderSupplier;
        mMenuButtonHelperSupplier = menuButtonHelperSupplier;
        mRoot = root;
        mBookmarkModelSupplier = bookmarkModelSupplier;
        mLocationBarModel = locationBarModel;
    }

    public void initializeWithNative(
            Activity activity,
            ResourceManager resourceManager,
            LayoutManagerImpl layoutManager,
            OnClickListener tabSwitcherListener,
            OnClickListener newTabClickListener,
            WindowAndroid windowAndroid,
            TabModelSelector tabModelSelector,
            IncognitoStateProvider incognitoStateProvider,
            ViewGroup topToolbarRoot,
            Runnable closeAllTabsAction) {
        if (BottomToolbarConfiguration.isBottomToolbarEnabled()) {
            mBottomToolbarCoordinator =
                    new BottomToolbarCoordinator(
                            mRoot,
                            mRoot.findViewById(R.id.bottom_toolbar),
                            mTabProvider,
                            mTabSwitcherLongclickListener,
                            mThemeColorProvider,
                            mOpenHomepageAction,
                            mSetUrlBarFocusAction,
                            mLayoutStateProviderSupplier,
                            mMenuButtonHelperSupplier,
                            mMediator,
                            mBookmarkModelSupplier,
                            mLocationBarModel);

            mBottomToolbarCoordinator.initializeWithNative(
                    tabSwitcherListener,
                    newTabClickListener,
                    tabModelSelector,
                    incognitoStateProvider,
                    topToolbarRoot,
                    closeAllTabsAction);
        }
    }

    @Override
    public void destroy() {
        super.destroy();

        if (mBottomToolbarCoordinator != null) mBottomToolbarCoordinator.destroy();
    }

    public void updateBookmarkButton(boolean isBookmarked, boolean editingAllowed) {
        if (mBottomToolbarCoordinator != null) {
            mBottomToolbarCoordinator.updateBookmarkButton(isBookmarked, editingAllowed);
        }
    }

    public void updateHomeButtonState() {
        if (mBottomToolbarCoordinator != null) {
            mBottomToolbarCoordinator.updateHomeButtonState();
        }
    }

    public void setBottomToolbarVisible(boolean visible) {
        if (mMediator instanceof BraveBottomControlsMediator) {
            ((BraveBottomControlsMediator) mMediator).setBottomToolbarVisible(visible);
        }
        if (mBottomToolbarCoordinator != null) {
            mBottomToolbarCoordinator.setBottomToolbarVisible(visible);
        }
    }

    public ObservableSupplierImpl<Boolean> getBottomToolbarVisibleSupplier() {
        if (mMediator instanceof BraveBottomControlsMediator) {
            return ((BraveBottomControlsMediator) mMediator).getBottomToolbarVisibleSupplier();
        }
        assert false : "Make sure mMediator is properly patched in bytecode.";
        return null;
    }

    public ObservableSupplierImpl<Boolean> getTabGroupUiVisibleSupplier() {
        if (mMediator instanceof BraveBottomControlsMediator) {
            return ((BraveBottomControlsMediator) mMediator).getTabGroupUiVisibleSupplier();
        }
        assert false : "Make sure mMediator is properly patched in bytecode.";
        return null;
    }
}
