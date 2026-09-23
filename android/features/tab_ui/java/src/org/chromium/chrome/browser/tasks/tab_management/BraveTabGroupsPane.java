/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tasks.tab_management;

import static org.chromium.build.NullUtil.assertNonNull;

import android.content.Context;

import org.chromium.base.supplier.LazyOneshotSupplier;
import org.chromium.base.supplier.MonotonicObservableSupplier;
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.data_sharing.DataSharingTabManager;
import org.chromium.chrome.browser.hub.LoadHint;
import org.chromium.chrome.browser.hub.PaneHubController;
import org.chromium.chrome.browser.hub.PaneId;
import org.chromium.chrome.browser.hub.PaneManager;
import org.chromium.chrome.browser.profiles.ProfileProvider;
import org.chromium.chrome.browser.tabmodel.TabModel;
import org.chromium.chrome.browser.ui.actions.button.DisplayButtonData;
import org.chromium.chrome.browser.ui.edge_to_edge.EdgeToEdgeController;
import org.chromium.components.tab_group_sync.TabGroupUiActionHandler;
import org.chromium.ui.modaldialog.ModalDialogManager;

import java.util.function.DoubleConsumer;
import java.util.function.Supplier;

/** Brave's {@link TabGroupsPane}. */
@NullMarked
public class BraveTabGroupsPane extends TabGroupsPane {
    private final DisplayButtonData mReferenceButtonData;
    private final Runnable mSettingsObserver = this::updateReferenceButton;

    private @Nullable PaneHubController mPaneHubController;
    private boolean mUnloading;

    BraveTabGroupsPane(
            Context context,
            LazyOneshotSupplier<TabModel> tabModelSupplier,
            DoubleConsumer onToolbarAlphaChange,
            OneshotSupplier<ProfileProvider> profileProviderSupplier,
            Supplier<PaneManager> paneManagerSupplier,
            Supplier<TabGroupUiActionHandler> tabGroupUiActionHandlerSupplier,
            Supplier<@Nullable ModalDialogManager> modalDialogManagerSupplier,
            MonotonicObservableSupplier<EdgeToEdgeController> edgeToEdgeSupplier,
            DataSharingTabManager dataSharingTabManager) {
        super(
                context,
                tabModelSupplier,
                onToolbarAlphaChange,
                profileProviderSupplier,
                paneManagerSupplier,
                tabGroupUiActionHandlerSupplier,
                modalDialogManagerSupplier,
                edgeToEdgeSupplier,
                dataSharingTabManager);

        mReferenceButtonData = assertNonNull(mReferenceButtonDataSupplier.get());
        BraveTabUiFeatureUtilities.addSettingsObserver(mSettingsObserver);
        updateReferenceButton();
    }

    @Override
    public void setPaneHubController(@Nullable PaneHubController paneHubController) {
        super.setPaneHubController(paneHubController);
        mPaneHubController = paneHubController;
    }

    @Override
    public void notifyLoadHint(@LoadHint int loadHint) {
        // TabGroupsPane releases its list by calling destroy(), which here would also drop the
        // settings observer. Unloading is not teardown: the button has to keep following the
        // switch while the pane is merely unloaded.
        mUnloading = loadHint == LoadHint.COLD;
        super.notifyLoadHint(loadHint);
        mUnloading = false;
    }

    @Override
    public void destroy() {
        if (!mUnloading) {
            BraveTabUiFeatureUtilities.removeSettingsObserver(mSettingsObserver);
        }
        super.destroy();
    }

    /**
     * Adds or removes this pane's reference button to follow the "Enable tab groups" master switch.
     * The Hub builds its pane list once per activity, so the pane itself stays registered and only
     * its button comes and goes: a pane without one gets no button in the pane switcher and {@link
     * PaneManager#focusPane} refuses to focus it, which is also how upstream drops the incognito
     * pane once the last incognito tab is closed.
     */
    private void updateReferenceButton() {
        boolean enabled = BraveTabUiFeatureUtilities.isTabGroupsEnabled();
        mReferenceButtonDataSupplier.set(enabled ? mReferenceButtonData : null);
        if (!enabled && mPaneHubController != null) {
            // The switch was turned off while this pane is the focused one, so it has to be left
            // behind. Reopening the Hub focuses the tab switcher on its own.
            mPaneHubController.focusPane(PaneId.TAB_SWITCHER);
        }
    }
}
