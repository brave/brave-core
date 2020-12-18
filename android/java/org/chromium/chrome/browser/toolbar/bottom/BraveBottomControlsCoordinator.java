/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.bottom;

import android.app.Activity;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnLongClickListener;
import android.view.ViewGroup;
import android.view.ViewStub;

import androidx.annotation.Nullable;

import org.chromium.base.Callback;
import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.OneShotCallback;
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.base.supplier.Supplier;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ActivityTabProvider;
import org.chromium.chrome.browser.browser_controls.BrowserControlsSizer;
import org.chromium.chrome.browser.compositor.layouts.LayoutManagerImpl;
import org.chromium.chrome.browser.fullscreen.FullscreenManager;
import org.chromium.chrome.browser.layouts.LayoutStateProvider;
import org.chromium.chrome.browser.share.ShareDelegate;
import org.chromium.chrome.browser.tabmodel.IncognitoStateProvider;
import org.chromium.chrome.browser.theme.ThemeColorProvider;
import org.chromium.chrome.browser.toolbar.HomeButton;
import org.chromium.chrome.browser.toolbar.TabCountProvider;
import org.chromium.chrome.browser.ui.appmenu.AppMenuButtonHelper;
import org.chromium.components.browser_ui.widget.scrim.ScrimCoordinator;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.resources.ResourceManager;

public class BraveBottomControlsCoordinator extends BottomControlsCoordinator {
    private BottomControlsMediator mMediator;

    private @Nullable BottomToolbarCoordinator mBottomToolbarCoordinator;
    private OnLongClickListener mTabSwitcherLongclickListener;
    private ActivityTabProvider mTabProvider;
    private ThemeColorProvider mThemeColorProvider;
    private ObservableSupplier<ShareDelegate> mShareDelegateSupplier;
    private ObservableSupplier<AppMenuButtonHelper> mMenuButtonHelperSupplier;
    private Runnable mOpenHomepageAction;
    private Callback<Integer> mSetUrlBarFocusAction;
    private OneshotSupplier<LayoutStateProvider> mLayoutStateProviderSupplier;
    private ScrollingBottomViewResourceFrameLayout mRoot;

    public BraveBottomControlsCoordinator(
            OneshotSupplier<LayoutStateProvider> layoutStateProviderSupplier,
            OnLongClickListener tabSwitcherLongclickListener, ActivityTabProvider tabProvider,
            BrowserControlsSizer controlsSizer, FullscreenManager fullscreenManager, ViewStub stub,
            ThemeColorProvider themeColorProvider,
            ObservableSupplier<ShareDelegate> shareDelegateSupplier,
            ObservableSupplier<AppMenuButtonHelper> menuButtonHelperSupplier,
            Runnable openHomepageAction, Callback<Integer> setUrlBarFocusAction,
            ScrimCoordinator scrimCoordinator,
            ObservableSupplier<Boolean> omniboxFocusStateSupplier) {
        super(controlsSizer, fullscreenManager, stub, themeColorProvider, shareDelegateSupplier,
                menuButtonHelperSupplier, openHomepageAction, setUrlBarFocusAction,
                scrimCoordinator, omniboxFocusStateSupplier);

        mTabSwitcherLongclickListener = tabSwitcherLongclickListener;
        mTabProvider = tabProvider;
        mThemeColorProvider = themeColorProvider;
        mShareDelegateSupplier = shareDelegateSupplier;
        mOpenHomepageAction = openHomepageAction;
        mSetUrlBarFocusAction = setUrlBarFocusAction;
        mLayoutStateProviderSupplier = layoutStateProviderSupplier;
        mMenuButtonHelperSupplier = menuButtonHelperSupplier;
    }

    public void setRootView(View root) {
        assert (root != null);
        mRoot = (ScrollingBottomViewResourceFrameLayout) root;
    }

    @Override
    public void initializeWithNative(Activity activity, ResourceManager resourceManager,
            LayoutManagerImpl layoutManager, OnClickListener tabSwitcherListener,
            OnClickListener newTabClickListener, WindowAndroid windowAndroid,
            TabCountProvider tabCountProvider, IncognitoStateProvider incognitoStateProvider,
            ViewGroup topToolbarRoot, Runnable closeAllTabsAction) {
        super.initializeWithNative(activity, resourceManager, layoutManager, tabSwitcherListener,
                newTabClickListener, windowAndroid, tabCountProvider, incognitoStateProvider,
                topToolbarRoot, closeAllTabsAction);

        if (BottomToolbarConfiguration.isBottomToolbarEnabled()) {
            mBottomToolbarCoordinator = new BottomToolbarCoordinator(mRoot,
                    mRoot.findViewById(R.id.bottom_toolbar_stub), mTabProvider,
                    mTabSwitcherLongclickListener, mThemeColorProvider, mShareDelegateSupplier,
                    mOpenHomepageAction, mSetUrlBarFocusAction, mLayoutStateProviderSupplier,
                    mMenuButtonHelperSupplier, mMediator);

            mBottomToolbarCoordinator.initializeWithNative(tabSwitcherListener, newTabClickListener,
                    tabCountProvider, incognitoStateProvider, topToolbarRoot, closeAllTabsAction);
        }
    }

    @Override
    public void setBottomControlsVisible(boolean isVisible) {
        super.setBottomControlsVisible(isVisible);

        if (mBottomToolbarCoordinator != null) {
            mBottomToolbarCoordinator.setBottomToolbarVisible(isVisible);
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
}
