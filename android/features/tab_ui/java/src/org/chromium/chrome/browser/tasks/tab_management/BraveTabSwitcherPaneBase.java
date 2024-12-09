/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tasks.tab_management;

import android.content.Context;

import androidx.annotation.NonNull;

import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.chrome.browser.hub.HubContainerView;
import org.chromium.chrome.browser.hub.HubLayoutAnimatorProvider;
import org.chromium.chrome.browser.profiles.ProfileProvider;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tasks.tab_management.TabListCoordinator.TabListMode;
import org.chromium.chrome.browser.ui.edge_to_edge.EdgeToEdgeController;
import org.chromium.chrome.browser.user_education.UserEducationHelper;
import org.chromium.chrome.tab_ui.R;

import java.util.function.DoubleConsumer;

public abstract class BraveTabSwitcherPaneBase extends TabSwitcherPaneBase {
    BraveTabSwitcherPaneBase(
            @NonNull Context context,
            @NonNull OneshotSupplier<ProfileProvider> profileProviderSupplier,
            @NonNull TabSwitcherPaneCoordinatorFactory factory,
            boolean isIncognito,
            @NonNull DoubleConsumer onToolbarAlphaChange,
            @NonNull UserEducationHelper userEducationHelper,
            @NonNull ObservableSupplier<EdgeToEdgeController> edgeToEdgeSupplier) {
        super(
                context,
                profileProviderSupplier,
                factory,
                isIncognito,
                onToolbarAlphaChange,
                userEducationHelper,
                edgeToEdgeSupplier);
        // These checks are to avoid resource not used warning. We use own own UI for the empty
        // state.
        assert R.drawable.phone_tab_switcher_empty_state_illustration_background > 0
                : "Something has changed in the upstream!";
        assert R.drawable.phone_tab_switcher_empty_state_illustration_bottom_window > 0
                : "Something has changed in the upstream!";
        assert R.drawable.phone_tab_switcher_empty_state_illustration_top_window > 0
                : "Something has changed in the upstream!";
    }

    @Override
    public @NonNull HubLayoutAnimatorProvider createHideHubLayoutAnimatorProvider(
            @NonNull HubContainerView hubContainerView) {
        int tabId = getCurrentTabId();
        if (getTabListMode() != TabListMode.LIST && tabId != Tab.INVALID_TAB_ID) {
            // Force call TabSwitcherPaneBase.createTabSwitcherPaneCoordinator
            // to ensure TabSwitcherPaneBase.mTabSwitcherPaneCoordinatorSupplier is set
            super.createTabSwitcherPaneCoordinator();
        }

        return super.createHideHubLayoutAnimatorProvider(hubContainerView);
    }
}
