/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.top;

import android.view.View;
import android.view.ViewStub;

import org.chromium.base.Callback;
import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.base.supplier.Supplier;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.browser_controls.BrowserStateBrowserControlsVisibilityDelegate;
import org.chromium.chrome.browser.fullscreen.FullscreenManager;
import org.chromium.chrome.browser.layouts.LayoutStateProvider;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.theme.ThemeColorProvider;
import org.chromium.chrome.browser.toolbar.ButtonData;
import org.chromium.chrome.browser.toolbar.ButtonDataProvider;
import org.chromium.chrome.browser.toolbar.ToolbarDataProvider;
import org.chromium.chrome.browser.toolbar.ToolbarTabController;
import org.chromium.chrome.browser.toolbar.menu_button.MenuButton;
import org.chromium.chrome.browser.toolbar.menu_button.MenuButtonCoordinator;
import org.chromium.chrome.browser.toolbar.top.NavigationPopup.HistoryDelegate;
import org.chromium.chrome.browser.toolbar.top.ToolbarTablet.OfflineDownloader;
import org.chromium.chrome.browser.ui.appmenu.AppMenuButtonHelper;
import org.chromium.chrome.browser.user_education.UserEducationHelper;
import org.chromium.content_public.browser.LoadUrlParams;
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

    public BraveTopToolbarCoordinator(ToolbarControlContainer controlContainer,
            ViewStub toolbarStub, ViewStub fullscreenToolbarStub, ToolbarLayout toolbarLayout,
            ToolbarDataProvider toolbarDataProvider, ToolbarTabController tabController,
            UserEducationHelper userEducationHelper, List<ButtonDataProvider> buttonDataProviders,
            OneshotSupplier<LayoutStateProvider> layoutStateProviderSupplier,
            ThemeColorProvider normalThemeColorProvider,
            ThemeColorProvider overviewThemeColorProvider,
            MenuButtonCoordinator browsingModeMenuButtonCoordinator,
            MenuButtonCoordinator overviewModeMenuButtonCoordinator,
            ObservableSupplier<AppMenuButtonHelper> appMenuButtonHelperSupplier,
            ObservableSupplier<TabModelSelector> tabModelSelectorSupplier,
            ObservableSupplier<Boolean> homepageEnabledSupplier,
            ButtonDataProvider identityDiscController, Callback<Runnable> invalidatorCallback,
            Supplier<ButtonData> identityDiscButtonSupplier,
            Supplier<ResourceManager> resourceManagerSupplier,
            BooleanSupplier isIncognitoModeEnabledSupplier, boolean isGridTabSwitcherEnabled,
            boolean isTabToGtsAnimationEnabled, boolean isStartSurfaceEnabled,
            boolean isTabGroupsAndroidContinuationEnabled, HistoryDelegate historyDelegate,
            BooleanSupplier partnerHomepageEnabledSupplier, OfflineDownloader offlineDownloader,
            boolean initializeWithIncognitoColors,
            Callback<LoadUrlParams> startSurfaceLogoClickedCallback,
            boolean isStartSurfaceRefactorEnabled, ObservableSupplier<Integer> constraintsSupplier,
            ObservableSupplier<Boolean> compositorInMotionSupplier,
            BrowserStateBrowserControlsVisibilityDelegate
                    browserStateBrowserControlsVisibilityDelegate,
            boolean shouldCreateLogoInStartToolbar, FullscreenManager fullscreenManager) {
        super(controlContainer, toolbarStub, fullscreenToolbarStub, toolbarLayout,
                toolbarDataProvider, tabController, userEducationHelper, buttonDataProviders,
                layoutStateProviderSupplier, normalThemeColorProvider, overviewThemeColorProvider,
                browsingModeMenuButtonCoordinator, overviewModeMenuButtonCoordinator,
                appMenuButtonHelperSupplier, tabModelSelectorSupplier, homepageEnabledSupplier,
                identityDiscController, invalidatorCallback, identityDiscButtonSupplier,
                resourceManagerSupplier, isIncognitoModeEnabledSupplier, isGridTabSwitcherEnabled,
                isTabToGtsAnimationEnabled, isStartSurfaceEnabled,
                isTabGroupsAndroidContinuationEnabled, historyDelegate,
                partnerHomepageEnabledSupplier, offlineDownloader, initializeWithIncognitoColors,
                startSurfaceLogoClickedCallback, isStartSurfaceRefactorEnabled, constraintsSupplier,
                compositorInMotionSupplier, browserStateBrowserControlsVisibilityDelegate,
                shouldCreateLogoInStartToolbar, fullscreenManager);

        mBraveToolbarLayout = toolbarLayout;
        mBraveMenuButtonCoordinator = browsingModeMenuButtonCoordinator;
        mConstraintsProxy = constraintsSupplier;

        if (isToolbarPhone()) {
            if (!isStartSurfaceEnabled) {
                mTabSwitcherModeCoordinator = new BraveTabSwitcherModeTTCoordinator(
                        controlContainer.getRootView().findViewById(R.id.tab_switcher_toolbar_stub),
                        fullscreenToolbarStub, overviewModeMenuButtonCoordinator,
                        isGridTabSwitcherEnabled, isTabToGtsAnimationEnabled,
                        isIncognitoModeEnabledSupplier, mToolbarColorObserverManager);
            }
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
    public void setTabSwitcherMode(boolean inTabSwitcherMode) {
        super.setTabSwitcherMode(inTabSwitcherMode);

        if (mBraveToolbarLayout instanceof ToolbarPhone) {
            mBraveToolbarLayout.setVisibility(
                    ((ToolbarPhone) mBraveToolbarLayout).isInTabSwitcherMode() ? View.INVISIBLE
                                                                               : View.VISIBLE);
        }
    }
}
