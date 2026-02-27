/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tasks.tab_management;

import android.content.Context;

import org.chromium.base.supplier.MonotonicObservableSupplier;
import org.chromium.base.supplier.NonNullObservableSupplier;
import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.compositor.CompositorViewHolder;
import org.chromium.chrome.browser.hub.HubContainerView;
import org.chromium.chrome.browser.hub.HubLayoutAnimatorProvider;
import org.chromium.chrome.browser.hub.PaneId;
import org.chromium.chrome.browser.ui.edge_to_edge.EdgeToEdgeController;
import org.chromium.chrome.browser.user_education.UserEducationHelper;

import java.util.function.DoubleConsumer;

@NullMarked
public abstract class BraveTabSwitcherPaneBase extends TabSwitcherPaneBase {
    BraveTabSwitcherPaneBase(
            @PaneId int paneId,
            Context context,
            TabSwitcherPaneCoordinatorFactory factory,
            boolean isIncognito,
            DoubleConsumer onToolbarAlphaChange,
            UserEducationHelper userEducationHelper,
            MonotonicObservableSupplier<EdgeToEdgeController> edgeToEdgeSupplier,
            MonotonicObservableSupplier<CompositorViewHolder> compositorViewHolderSupplier,
            TabGroupCreationUiDelegate tabGroupCreationUiDelegate,
            NonNullObservableSupplier<Boolean> xrSpaceModeObservableSupplier) {
        super(
                paneId,
                context,
                factory,
                isIncognito,
                onToolbarAlphaChange,
                userEducationHelper,
                edgeToEdgeSupplier,
                compositorViewHolderSupplier,
                tabGroupCreationUiDelegate,
                xrSpaceModeObservableSupplier);
    }

    @Override
    public HubLayoutAnimatorProvider createHideHubLayoutAnimatorProvider(
            HubContainerView hubContainerView) {
        if (getTabSwitcherPaneCoordinator() == null && getCurrentTab() != null) {
            // Force call TabSwitcherPaneBase.createTabSwitcherPaneCoordinator
            // to ensure TabSwitcherPaneBase.mTabSwitcherPaneCoordinatorSupplier is set
            super.createTabSwitcherPaneCoordinator();
        }

        return super.createHideHubLayoutAnimatorProvider(hubContainerView);
    }
}
