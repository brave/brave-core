/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.top;

import android.content.Context;

import org.chromium.base.Callback;
import org.chromium.base.supplier.BooleanSupplier;
import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.base.supplier.Supplier;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.identity_disc.IdentityDiscController;
import org.chromium.chrome.browser.layouts.LayoutStateProvider;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.theme.ThemeColorProvider;
import org.chromium.chrome.browser.toolbar.ButtonData;
import org.chromium.chrome.browser.toolbar.ButtonDataProvider;
import org.chromium.chrome.browser.toolbar.ToolbarDataProvider;
import org.chromium.chrome.browser.toolbar.ToolbarTabController;
import org.chromium.chrome.browser.toolbar.menu_button.MenuButtonCoordinator;
import org.chromium.chrome.browser.ui.appmenu.AppMenuButtonHelper;
import org.chromium.chrome.browser.user_education.UserEducationHelper;
import org.chromium.chrome.features.start_surface.StartSurface;
import org.chromium.chrome.features.start_surface.StartSurfaceConfiguration;
import org.chromium.ui.resources.ResourceManager;

import java.util.List;

public class BraveTopToolbarCoordinator extends TopToolbarCoordinator {
    private TabSwitcherModeTTCoordinatorPhone mTabSwitcherModeCoordinatorPhone;
    private OptionalBrowsingModeButtonController mOptionalButtonController;
    private ToolbarLayout mBraveToolbarLayout;

    public BraveTopToolbarCoordinator(ToolbarControlContainer controlContainer,
            ToolbarLayout toolbarLayout, ToolbarDataProvider toolbarDataProvider,
            ToolbarTabController tabController, UserEducationHelper userEducationHelper,
            List<ButtonDataProvider> buttonDataProviders,
            OneshotSupplier<LayoutStateProvider> layoutStateProviderSupplier,
            ThemeColorProvider normalThemeColorProvider,
            ThemeColorProvider overviewThemeColorProvider,
            MenuButtonCoordinator browsingModeMenuButtonCoordinator,
            MenuButtonCoordinator overviewModeMenuButtonCoordinator,
            ObservableSupplier<AppMenuButtonHelper> appMenuButtonHelperSupplier,
            ObservableSupplier<TabModelSelector> tabModelSelectorSupplier,
            ObservableSupplier<Boolean> homeButtonVisibilitySupplier,
            ObservableSupplier<Boolean> homepageManagedByPolicySupplier,
            ObservableSupplier<Boolean> identityDiscStateSupplier,
            Callback<Runnable> invalidatorCallback, Supplier<ButtonData> identityDiscButtonSupplier,
            OneshotSupplier<StartSurface> startSurfaceSupplier,
            Supplier<ResourceManager> resourceManagerSupplier, BooleanSupplier isInVrSupplier,
            boolean isGridTabSwitcherEnabled, boolean isTabToGtsAnimationEnabled,
            boolean isStartSurfaceEnabled) {
        super(controlContainer, toolbarLayout, toolbarDataProvider, tabController,
                userEducationHelper, buttonDataProviders, layoutStateProviderSupplier,
                normalThemeColorProvider, overviewThemeColorProvider,
                browsingModeMenuButtonCoordinator, overviewModeMenuButtonCoordinator,
                appMenuButtonHelperSupplier, tabModelSelectorSupplier, homeButtonVisibilitySupplier,
                homepageManagedByPolicySupplier, identityDiscStateSupplier, invalidatorCallback,
                identityDiscButtonSupplier, startSurfaceSupplier, resourceManagerSupplier,
                isInVrSupplier, isGridTabSwitcherEnabled, isTabToGtsAnimationEnabled,
                isStartSurfaceEnabled);

        mBraveToolbarLayout = toolbarLayout;

        if (isToolbarPhone()) {
            if (!StartSurfaceConfiguration.isStartSurfaceEnabled()) {
                mTabSwitcherModeCoordinatorPhone = new BraveTabSwitcherModeTTCoordinatorPhone(
                        controlContainer.getRootView().findViewById(R.id.tab_switcher_toolbar_stub),
                        overviewModeMenuButtonCoordinator, isGridTabSwitcherEnabled,
                        isTabToGtsAnimationEnabled, isStartSurfaceEnabled);
            }
        }
    }

    public void onBottomToolbarVisibilityChanged(boolean isVisible) {
        if (mBraveToolbarLayout instanceof BraveToolbarLayout) {
            ((BraveToolbarLayout) mBraveToolbarLayout).onBottomToolbarVisibilityChanged(isVisible);
        }
        if (mTabSwitcherModeCoordinatorPhone instanceof BraveTabSwitcherModeTTCoordinatorPhone) {
            ((BraveTabSwitcherModeTTCoordinatorPhone) mTabSwitcherModeCoordinatorPhone)
                    .onBottomToolbarVisibilityChanged(isVisible);
        }
        mOptionalButtonController.updateButtonVisibility();
    }

    public boolean isToolbarPhone() {
        return mBraveToolbarLayout instanceof ToolbarPhone;
    }
}
