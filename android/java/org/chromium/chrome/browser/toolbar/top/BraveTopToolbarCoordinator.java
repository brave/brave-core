/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.top;

import android.content.Context;

import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ThemeColorProvider;
import org.chromium.chrome.browser.compositor.layouts.OverviewModeBehavior;
import org.chromium.chrome.browser.identity_disc.IdentityDiscController;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.toolbar.ButtonDataProvider;
import org.chromium.chrome.browser.toolbar.ToolbarDataProvider;
import org.chromium.chrome.browser.toolbar.ToolbarTabController;
import org.chromium.chrome.browser.toolbar.menu_button.MenuButtonCoordinator;
import org.chromium.chrome.browser.ui.appmenu.AppMenuButtonHelper;
import org.chromium.chrome.browser.user_education.UserEducationHelper;
import org.chromium.chrome.features.start_surface.StartSurfaceConfiguration;

import java.util.List;

public class BraveTopToolbarCoordinator extends TopToolbarCoordinator {
    private TabSwitcherModeTTCoordinatorPhone mTabSwitcherModeCoordinatorPhone;
    private OptionalBrowsingModeButtonController mOptionalButtonController;

    public BraveTopToolbarCoordinator(ToolbarControlContainer controlContainer,
            ToolbarLayout toolbarLayout, IdentityDiscController identityDiscController,
            ToolbarDataProvider toolbarDataProvider, ToolbarTabController tabController,
            UserEducationHelper userEducationHelper, List<ButtonDataProvider> buttonDataProviders,
            OneshotSupplier<OverviewModeBehavior> overviewModeBehaviorSupplier,
            ThemeColorProvider normalThemeColorProvider,
            ThemeColorProvider overviewThemeColorProvider,
            MenuButtonCoordinator browsingModeMenuButtonCoordinator,
            MenuButtonCoordinator startSurfaceMenuButtonCoordinator,
            ObservableSupplier<AppMenuButtonHelper> appMenuButtonHelperSupplier, Context context,
            ObservableSupplier<TabModelSelector> tabModelSelectorSupplier) {
        super(controlContainer, toolbarLayout, identityDiscController, toolbarDataProvider,
                tabController, userEducationHelper, buttonDataProviders,
                overviewModeBehaviorSupplier, normalThemeColorProvider, overviewThemeColorProvider,
                browsingModeMenuButtonCoordinator, startSurfaceMenuButtonCoordinator,
                appMenuButtonHelperSupplier, context, tabModelSelectorSupplier);

        if (toolbarLayout instanceof ToolbarPhone) {
            if (!StartSurfaceConfiguration.isStartSurfaceEnabled()) {
                mTabSwitcherModeCoordinatorPhone = new BraveTabSwitcherModeTTCoordinatorPhone(
                        controlContainer.getRootView().findViewById(
                                R.id.tab_switcher_toolbar_stub));
            }
        }
    }

    public void onBottomToolbarVisibilityChanged(boolean isVisible) {
        if (mTabSwitcherModeCoordinatorPhone instanceof BraveTabSwitcherModeTTCoordinatorPhone) {
            ((BraveTabSwitcherModeTTCoordinatorPhone) mTabSwitcherModeCoordinatorPhone)
                    .onBottomToolbarVisibilityChanged(isVisible);
        }
        mOptionalButtonController.updateButtonVisibility();
    }
}
