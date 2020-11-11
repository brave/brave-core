/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tabbed_mode;

import org.chromium.base.Callback;
import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.ObservableSupplierImpl;
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.base.supplier.Supplier;
import org.chromium.chrome.browser.ActivityTabProvider;
import org.chromium.chrome.browser.app.ChromeActivity;
import org.chromium.chrome.browser.bookmarks.BookmarkBridge;
import org.chromium.chrome.browser.compositor.bottombar.ephemeraltab.EphemeralTabCoordinator;
import org.chromium.chrome.browser.compositor.layouts.OverviewModeBehavior;
import org.chromium.chrome.browser.contextualsearch.ContextualSearchManager;
import org.chromium.chrome.browser.intent.IntentMetadata;
import org.chromium.chrome.browser.layouts.LayoutStateProvider;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.share.ShareDelegate;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.toolbar.bottom.BottomToolbarConfiguration;
import org.chromium.chrome.features.start_surface.StartSurface;

public class BraveTabbedRootUiCoordinator extends TabbedRootUiCoordinator {
    public BraveTabbedRootUiCoordinator(ChromeActivity activity,
            Callback<Boolean> onOmniboxFocusChangedListener,
            OneshotSupplier<IntentMetadata> intentMetadataOneshotSupplier,
            ObservableSupplier<ShareDelegate> shareDelegateSupplier,
            ActivityTabProvider tabProvider,
            ObservableSupplierImpl<EphemeralTabCoordinator> ephemeralTabCoordinatorSupplier,
            ObservableSupplier<Profile> profileSupplier,
            ObservableSupplier<BookmarkBridge> bookmarkBridgeSupplier,
            OneshotSupplier<OverviewModeBehavior> overviewModeBehaviorSupplier,
            Supplier<ContextualSearchManager> contextualSearchManagerSupplier,
            ObservableSupplier<TabModelSelector> tabModelSelectorSupplier,
            OneshotSupplier<StartSurface> startSurfaceSupplier,
            OneshotSupplier<LayoutStateProvider> layoutStateProviderOneshotSupplier) {
        super(activity, onOmniboxFocusChangedListener, intentMetadataOneshotSupplier,
                shareDelegateSupplier, tabProvider, ephemeralTabCoordinatorSupplier,
                profileSupplier, bookmarkBridgeSupplier, overviewModeBehaviorSupplier,
                contextualSearchManagerSupplier, tabModelSelectorSupplier, startSurfaceSupplier,
                layoutStateProviderOneshotSupplier);
    }

    @Override
    protected void initializeToolbar() {
        super.initializeToolbar();

        if (BottomToolbarConfiguration.isBottomToolbarEnabled()) {
            getToolbarManager().enableBottomControls();
        }
    }
}
