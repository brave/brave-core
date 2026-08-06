/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tasks.tab_management;

import android.content.Context;

import androidx.recyclerview.widget.RecyclerView;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.tabmodel.TabModel;
import org.chromium.chrome.browser.tasks.tab_management.TabListMediator.TabGridDialogHandler;
import org.chromium.chrome.browser.tasks.tab_management.TabListMediator.TabListLayoutType;

import java.util.function.Supplier;

/**
 * Brave's {@link TabGridItemTouchHelperCallback}. Instantiated in place of the upstream class via a
 * plaster redirect.
 */
@NullMarked
public class BraveTabGridItemTouchHelperCallback extends TabGridItemTouchHelperCallback {
    private final @TabListLayoutType int mUpstreamLayoutType;

    public BraveTabGridItemTouchHelperCallback(
            Context context,
            TabGroupCreationDialogManager tabGroupCreationDialogManager,
            TabListModel tabListModel,
            Supplier<TabModel> currentTabModelSupplier,
            TabActionListener tabClosedListener,
            @Nullable TabGridDialogHandler tabGridDialogHandler,
            String componentName,
            @TabListLayoutType int layoutType,
            Runnable onDragStateChangedListener) {
        super(
                context,
                tabGroupCreationDialogManager,
                tabListModel,
                currentTabModelSupplier,
                tabClosedListener,
                tabGridDialogHandler,
                componentName,
                layoutType,
                onDragStateChangedListener);
        mUpstreamLayoutType = layoutType;
        updateLayoutType();
    }

    @Override
    public void onSelectedChanged(RecyclerView.@Nullable ViewHolder viewHolder, int actionState) {
        updateLayoutType();
        super.onSelectedChanged(viewHolder, actionState);
    }

    /**
     * Uses the FLAT (non-grouping) drag layout while the "Enable tab groups" master switch is off:
     * tabs can still be reordered by dragging, but dragging one tab over another does nothing (no
     * hover-to-merge, no group creation).
     *
     * <p>The preference is re-read for every drag instead of only in the constructor, so toggling
     * the switch in settings applies to the next drag without a browser restart.
     */
    private void updateLayoutType() {
        mLayoutType =
                BraveTabUiFeatureUtilities.isTabGroupsEnabled()
                        ? mUpstreamLayoutType
                        : TabListLayoutType.FLAT;
    }
}
