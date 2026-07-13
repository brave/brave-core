/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.compositor.overlays.strip;

import android.app.Activity;

import org.chromium.base.supplier.MonotonicObservableSupplier;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.bookmarks.TabBookmarker;
import org.chromium.chrome.browser.multiwindow.MultiInstanceManager;
import org.chromium.chrome.browser.share.ShareDelegate;
import org.chromium.chrome.browser.tabmodel.TabGroupUtils.TabGroupCreationCallback;
import org.chromium.chrome.browser.tabmodel.TabModel;
import org.chromium.chrome.browser.tasks.tab_management.BraveTabUiFeatureUtilities;
import org.chromium.chrome.browser.tasks.tab_management.TabGroupListBottomSheetCoordinator;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager;
import org.chromium.chrome.tab_ui.R;
import org.chromium.components.collaboration.CollaborationService;
import org.chromium.components.tab_group_sync.TabGroupSyncService;
import org.chromium.ui.base.ActivityResultTracker;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.modaldialog.ModalDialogManager;
import org.chromium.ui.modelutil.MVCListAdapter.ModelList;

import java.util.function.BiConsumer;
import java.util.function.Supplier;

/**
 * Brave's {@link TabContextMenuCoordinator}. Removes the "Add/Move to group" entries from the
 * tablet tab strip context menu when the Brave "Enable tab groups" master switch is off.
 * Instantiated in place of the upstream class via a plaster redirect.
 */
@NullMarked
public class BraveTabContextMenuCoordinator extends TabContextMenuCoordinator {
    protected BraveTabContextMenuCoordinator(
            Supplier<TabModel> tabModelSupplier,
            TabGroupListBottomSheetCoordinator tabGroupListBottomSheetCoordinator,
            TabGroupCreationCallback tabGroupCreationCallback,
            MultiInstanceManager multiInstanceManager,
            MonotonicObservableSupplier<ShareDelegate> shareDelegateSupplier,
            WindowAndroid windowAndroid,
            Activity activity,
            @Nullable TabGroupSyncService tabGroupSyncService,
            CollaborationService collaborationService,
            Supplier<TabBookmarker> tabBookmarkerSupplier,
            BiConsumer<AnchorInfo, Boolean> reorderFunction,
            SnackbarManager snackbarManager,
            @Nullable ActivityResultTracker activityResultTracker,
            @Nullable ModalDialogManager modalDialogManager) {
        super(
                tabModelSupplier,
                tabGroupListBottomSheetCoordinator,
                tabGroupCreationCallback,
                multiInstanceManager,
                shareDelegateSupplier,
                windowAndroid,
                activity,
                tabGroupSyncService,
                collaborationService,
                tabBookmarkerSupplier,
                reorderFunction,
                snackbarManager,
                activityResultTracker,
                modalDialogManager);
    }

    @Override
    protected void buildMenuActionItems(ModelList itemList, AnchorInfo anchorInfo) {
        super.buildMenuActionItems(itemList, anchorInfo);
        if (!BraveTabUiFeatureUtilities.isTabGroupsEnabled()) {
            BraveTabUiFeatureUtilities.removeMenuItems(
                    itemList, R.id.add_to_new_tab_group, R.id.add_to_tab_group);
        }
    }
}
