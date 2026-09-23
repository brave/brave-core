/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tasks.tab_management;

import static org.chromium.build.NullUtil.assumeNonNull;

import android.app.Activity;
import android.view.ViewGroup;

import androidx.annotation.VisibleForTesting;

import org.chromium.base.supplier.MonotonicObservableSupplier;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.browser_controls.BrowserControlsStateProvider;
import org.chromium.chrome.browser.tab_ui.TabContentManager;
import org.chromium.chrome.browser.tab_ui.TabListMode;
import org.chromium.chrome.browser.tabmodel.TabModel;
import org.chromium.chrome.browser.tasks.tab_management.TabListEditorCoordinator.TabListEditorController;
import org.chromium.chrome.browser.ui.edge_to_edge.EdgeToEdgeController;
import org.chromium.components.browser_ui.bottomsheet.BottomSheetController;
import org.chromium.components.browser_ui.desktop_windowing.DesktopWindowStateManager;
import org.chromium.ui.modaldialog.ModalDialogManager;

import java.util.ArrayList;
import java.util.List;

/**
 * Brave's {@link TabListEditorManager}. Instantiated in place of the upstream class via a plaster
 * redirect.
 */
@NullMarked
public class BraveTabListEditorManager extends TabListEditorManager {
    public BraveTabListEditorManager(
            Activity activity,
            ModalDialogManager modalDialogManager,
            ViewGroup coordinatorView,
            ViewGroup rootView,
            BrowserControlsStateProvider browserControlsStateProvider,
            MonotonicObservableSupplier<TabModel> currentTabModelSupplier,
            TabContentManager tabContentManager,
            TabListCoordinator tabListCoordinator,
            BottomSheetController bottomSheetController,
            @TabListMode int mode,
            @Nullable Runnable onTabGroupCreation,
            @Nullable DesktopWindowStateManager desktopWindowStateManager,
            MonotonicObservableSupplier<EdgeToEdgeController> edgeToEdgeSupplier) {
        super(
                activity,
                modalDialogManager,
                coordinatorView,
                rootView,
                browserControlsStateProvider,
                currentTabModelSupplier,
                tabContentManager,
                tabListCoordinator,
                bottomSheetController,
                mode,
                onTabGroupCreation,
                desktopWindowStateManager,
                edgeToEdgeSupplier);
    }

    @Override
    public void showTabListEditor() {
        super.showTabListEditor();

        List<TabListEditorAction> actions = filterActions(assumeNonNull(mTabListEditorActions));
        if (actions == null) return;

        TabListEditorController controller = getControllerSupplier().get();
        assumeNonNull(controller);
        controller.configureToolbarWithMenuItems(actions);
    }

    /**
     * Returns a copy of {@code actions} without the "Add tabs to new group" action when the "Enable
     * tab groups" master switch is off, or null when upstream's list can be used as is.
     *
     * <p>Upstream builds its action list once and caches it, so the list itself is left alone and
     * the toolbar is reconfigured with a copy on every show. That keeps the switch live, without a
     * browser restart.
     */
    @VisibleForTesting
    static @Nullable List<TabListEditorAction> filterActions(List<TabListEditorAction> actions) {
        if (BraveTabUiFeatureUtilities.isTabGroupsEnabled()) return null;

        List<TabListEditorAction> filtered = new ArrayList<>(actions);
        return filtered.removeIf(action -> action instanceof TabListEditorAddToGroupAction)
                ? filtered
                : null;
    }
}
