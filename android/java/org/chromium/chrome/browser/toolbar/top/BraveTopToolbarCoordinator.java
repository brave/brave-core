/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.top;

import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewStub;

import androidx.annotation.Nullable;

import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.base.supplier.Supplier;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.browser_controls.BrowserControlsVisibilityManager;
import org.chromium.chrome.browser.browser_controls.BrowserStateBrowserControlsVisibilityDelegate;
import org.chromium.chrome.browser.fullscreen.FullscreenManager;
import org.chromium.chrome.browser.layouts.LayoutManager;
import org.chromium.chrome.browser.layouts.LayoutStateProvider;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabObscuringHandler;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.theme.ThemeColorProvider;
import org.chromium.chrome.browser.theme.TopUiThemeColorProvider;
import org.chromium.chrome.browser.toolbar.ButtonDataProvider;
import org.chromium.chrome.browser.toolbar.ToolbarDataProvider;
import org.chromium.chrome.browser.toolbar.ToolbarTabController;
import org.chromium.chrome.browser.toolbar.menu_button.MenuButton;
import org.chromium.chrome.browser.toolbar.menu_button.MenuButtonCoordinator;
import org.chromium.chrome.browser.toolbar.top.NavigationPopup.HistoryDelegate;
import org.chromium.chrome.browser.toolbar.top.ToolbarTablet.OfflineDownloader;
import org.chromium.chrome.browser.toolbar.top.tab_strip.TabStripTransitionCoordinator.TabStripTransitionDelegate;
import org.chromium.chrome.browser.ui.appmenu.AppMenuButtonHelper;
import org.chromium.chrome.browser.ui.appmenu.AppMenuDelegate;
import org.chromium.chrome.browser.ui.desktop_windowing.DesktopWindowStateProvider;
import org.chromium.chrome.browser.user_education.UserEducationHelper;
import org.chromium.ui.resources.ResourceManager;

import java.util.List;
import java.util.function.BooleanSupplier;

public class BraveTopToolbarCoordinator extends TopToolbarCoordinator {
    // To delete in bytecode. Variables from the parent class will be used instead.
    private OptionalBrowsingModeButtonController mOptionalButtonController;
    private TabSwitcherModeTTCoordinator mTabSwitcherModeCoordinator;
    private ToolbarColorObserverManager mToolbarColorObserverManager;

    // Own members.
    private ToolbarLayout mBraveToolbarLayout;
    private MenuButtonCoordinator mBraveMenuButtonCoordinator;
    private boolean mIsBottomToolbarVisible;
    private ObservableSupplier<Integer> mConstraintsProxy;
    private ObservableSupplier<TabModelSelector> mTabModelSelectorSupplier;
    private ToolbarControlContainer mControlContainer;
    private boolean mInTabSwitcherMode;

    public BraveTopToolbarCoordinator(
            ToolbarControlContainer controlContainer,
            ViewStub toolbarStub,
            ToolbarLayout toolbarLayout,
            ToolbarDataProvider toolbarDataProvider,
            ToolbarTabController tabController,
            UserEducationHelper userEducationHelper,
            List<ButtonDataProvider> buttonDataProviders,
            OneshotSupplier<LayoutStateProvider> layoutStateProviderSupplier,
            ThemeColorProvider normalThemeColorProvider,
            MenuButtonCoordinator browsingModeMenuButtonCoordinator,
            MenuButtonCoordinator overviewModeMenuButtonCoordinator,
            ObservableSupplier<AppMenuButtonHelper> appMenuButtonHelperSupplier,
            ObservableSupplier<TabModelSelector> tabModelSelectorSupplier,
            ObservableSupplier<Boolean> homepageEnabledSupplier,
            Supplier<ResourceManager> resourceManagerSupplier,
            BooleanSupplier isIncognitoModeEnabledSupplier,
            HistoryDelegate historyDelegate,
            BooleanSupplier partnerHomepageEnabledSupplier,
            OfflineDownloader offlineDownloader,
            boolean initializeWithIncognitoColors,
            ObservableSupplier<Integer> constraintsSupplier,
            ObservableSupplier<Boolean> compositorInMotionSupplier,
            BrowserStateBrowserControlsVisibilityDelegate
                    browserStateBrowserControlsVisibilityDelegate,
            FullscreenManager fullscreenManager,
            TabObscuringHandler tabObscuringHandler,
            @Nullable DesktopWindowStateProvider desktopWindowStateProvider,
            OneshotSupplier<TabStripTransitionDelegate> tabStripTransitionDelegateSupplier) {
        super(
                controlContainer,
                toolbarStub,
                toolbarLayout,
                toolbarDataProvider,
                tabController,
                userEducationHelper,
                buttonDataProviders,
                layoutStateProviderSupplier,
                normalThemeColorProvider,
                browsingModeMenuButtonCoordinator,
                overviewModeMenuButtonCoordinator,
                appMenuButtonHelperSupplier,
                tabModelSelectorSupplier,
                homepageEnabledSupplier,
                resourceManagerSupplier,
                isIncognitoModeEnabledSupplier,
                historyDelegate,
                partnerHomepageEnabledSupplier,
                offlineDownloader,
                initializeWithIncognitoColors,
                constraintsSupplier,
                compositorInMotionSupplier,
                browserStateBrowserControlsVisibilityDelegate,
                fullscreenManager,
                tabObscuringHandler,
                desktopWindowStateProvider,
                tabStripTransitionDelegateSupplier);

        mBraveToolbarLayout = toolbarLayout;
        mBraveMenuButtonCoordinator = browsingModeMenuButtonCoordinator;
        mConstraintsProxy = constraintsSupplier;
        mTabModelSelectorSupplier = tabModelSelectorSupplier;
        mControlContainer = controlContainer;

        if (isToolbarPhone()) {
            mTabSwitcherModeCoordinator =
                    new BraveTabSwitcherModeTTCoordinator(
                            controlContainer
                                    .getRootView()
                                    .findViewById(R.id.tab_switcher_toolbar_stub),
                            overviewModeMenuButtonCoordinator,
                            isIncognitoModeEnabledSupplier,
                            mToolbarColorObserverManager);
        }
    }

    public void onBottomToolbarVisibilityChanged(boolean isVisible) {
        mIsBottomToolbarVisible = isVisible;
        if (mBraveToolbarLayout instanceof BraveToolbarLayout) {
            ((BraveToolbarLayoutImpl) mBraveToolbarLayout)
                    .onBottomToolbarVisibilityChanged(isVisible);
        }
        if (mTabSwitcherModeCoordinator instanceof BraveTabSwitcherModeTTCoordinator) {
            ((BraveTabSwitcherModeTTCoordinator) mTabSwitcherModeCoordinator)
                    .onBottomToolbarVisibilityChanged(isVisible);
        }
        mOptionalButtonController.updateButtonVisibility();
    }

    public boolean isToolbarPhone() {
        return mBraveToolbarLayout instanceof ToolbarPhone;
    }

    @Override
    public MenuButton getMenuButtonWrapper() {
        // We consider that there is no top toolbar menu button, if bottom toolbar is visible.
        return mIsBottomToolbarVisible ? null : mBraveMenuButtonCoordinator.getMenuButton();
    }

    public ObservableSupplier<Integer> getConstraintsProxy() {
        return mConstraintsProxy;
    }

    @Override
    public void initializeWithNative(
            Profile profile,
            Runnable layoutUpdater,
            OnClickListener tabSwitcherClickHandler,
            OnClickListener newTabClickHandler,
            OnClickListener bookmarkClickHandler,
            OnClickListener customTabsBackClickHandler,
            AppMenuDelegate appMenuDelegate,
            LayoutManager layoutManager,
            ObservableSupplier<Tab> tabSupplier,
            BrowserControlsVisibilityManager browserControlsVisibilityManager,
            TopUiThemeColorProvider topUiThemeColorProvider) {
        super.initializeWithNative(
                profile,
                layoutUpdater,
                tabSwitcherClickHandler,
                newTabClickHandler,
                bookmarkClickHandler,
                customTabsBackClickHandler,
                appMenuDelegate,
                layoutManager,
                tabSupplier,
                browserControlsVisibilityManager,
                topUiThemeColorProvider);

        assert mBraveToolbarLayout instanceof BraveToolbarLayoutImpl
                : "Something has changed in the upstream!";
        if (mBraveToolbarLayout instanceof BraveToolbarLayoutImpl) {
            ((BraveToolbarLayoutImpl) mBraveToolbarLayout)
                    .setTabModelSelector(mTabModelSelectorSupplier.get());
        }
    }

    @Override
    public void setTabSwitcherMode(boolean inTabSwitcherMode) {
        mInTabSwitcherMode = inTabSwitcherMode;

        super.setTabSwitcherMode(inTabSwitcherMode);
    }

    @Override
    public void onTabSwitcherTransitionFinished() {
        super.onTabSwitcherTransitionFinished();

        if (isToolbarPhone() && mInTabSwitcherMode) {
            // Make sure we have proper state at the end of the tab switcher transition.
            mControlContainer.setVisibility(View.VISIBLE);
        }
    }
}
