/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tasks.tab_management;

import android.app.Activity;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.bookmarks.TabBookmarker;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.share.ShareDelegate;
import org.chromium.chrome.browser.tabmodel.TabModel;
import org.chromium.chrome.tab_ui.R;
import org.chromium.components.collaboration.CollaborationService;
import org.chromium.components.tab_group_sync.TabGroupSyncService;
import org.chromium.ui.modelutil.MVCListAdapter.ModelList;

import java.util.function.Supplier;

/**
 * Brave's {@link TabGridContextMenuCoordinator}. Removes the tab group creation entries ("Add to
 * new group"/"Add to group") from the tab grid long-press context menu when the Brave "Enable tab
 * groups" master switch is off. Instantiated in place of the upstream class via a plaster redirect.
 */
@NullMarked
public class BraveTabGridContextMenuCoordinator extends TabGridContextMenuCoordinator {
    public BraveTabGridContextMenuCoordinator(
            Activity activity,
            Supplier<@Nullable TabBookmarker> tabBookmarkerSupplier,
            Profile profile,
            TabModel tabModel,
            TabGroupListBottomSheetCoordinator tabGroupListBottomSheetCoordinator,
            TabGroupCreationDialogManager tabGroupCreationDialogManager,
            Supplier<@Nullable ShareDelegate> shareDelegateSupplier,
            @Nullable TabGroupSyncService tabGroupSyncService,
            CollaborationService collaborationService,
            ShowTabListEditor showTabListEditor) {
        super(
                activity,
                tabBookmarkerSupplier,
                profile,
                tabModel,
                tabGroupListBottomSheetCoordinator,
                tabGroupCreationDialogManager,
                shareDelegateSupplier,
                tabGroupSyncService,
                collaborationService,
                showTabListEditor);
    }

    @Override
    protected void buildMenuActionItems(ModelList itemList, Integer id) {
        super.buildMenuActionItems(itemList, id);
        if (!BraveTabUiFeatureUtilities.isTabGroupsEnabled()) {
            BraveTabUiFeatureUtilities.removeMenuItems(
                    itemList, R.id.add_to_new_tab_group, R.id.add_to_tab_group);
        }
    }
}
