/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tasks.tab_management;

import android.content.Context;

import androidx.annotation.NonNull;

import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.chrome.browser.compositor.CompositorViewHolder;
import org.chromium.chrome.browser.hub.HubContainerView;
import org.chromium.chrome.browser.hub.HubLayoutAnimatorProvider;
import org.chromium.chrome.browser.ui.edge_to_edge.EdgeToEdgeController;
import org.chromium.chrome.browser.user_education.UserEducationHelper;

import java.util.function.DoubleConsumer;

public abstract class BraveTabSwitcherPaneBase extends TabSwitcherPaneBase {
    BraveTabSwitcherPaneBase(
            @NonNull Context context,
            @NonNull TabSwitcherPaneCoordinatorFactory factory,
            boolean isIncognito,
            @NonNull DoubleConsumer onToolbarAlphaChange,
            @NonNull UserEducationHelper userEducationHelper,
            @NonNull ObservableSupplier<EdgeToEdgeController> edgeToEdgeSupplier,
            @NonNull ObservableSupplier<CompositorViewHolder> compositorViewHolderSupplier,
            @NonNull TabGroupCreationUiDelegate tabGroupCreationUiDelegate) {
        super(
                context,
                factory,
                isIncognito,
                onToolbarAlphaChange,
                userEducationHelper,
                edgeToEdgeSupplier,
                compositorViewHolderSupplier,
                tabGroupCreationUiDelegate);
    }

    @Override
    public @NonNull HubLayoutAnimatorProvider createHideHubLayoutAnimatorProvider(
            @NonNull HubContainerView hubContainerView) {
        if (!getTabSwitcherPaneCoordinatorSupplier().hasValue() && getCurrentTab() != null) {
            // Force call TabSwitcherPaneBase.createTabSwitcherPaneCoordinator
            // to ensure TabSwitcherPaneBase.mTabSwitcherPaneCoordinatorSupplier is set
            super.createTabSwitcherPaneCoordinator();
        }

        return super.createHideHubLayoutAnimatorProvider(hubContainerView);
    }
}
