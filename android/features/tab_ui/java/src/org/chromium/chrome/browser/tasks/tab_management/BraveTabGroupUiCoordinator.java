/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tasks.tab_management;

import android.app.Activity;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;

import org.chromium.base.Callback;
import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.chrome.browser.browser_controls.BrowserControlsStateProvider;
import org.chromium.chrome.browser.data_sharing.DataSharingTabManager;
import org.chromium.chrome.browser.layouts.LayoutStateProvider;
import org.chromium.chrome.browser.tab_ui.TabContentManager;
import org.chromium.chrome.browser.tabmodel.TabCreatorManager;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.tabmodel.TabModelSelectorObserver;
import org.chromium.chrome.browser.theme.ThemeColorProvider;
import org.chromium.chrome.browser.toolbar.bottom.BottomControlsCoordinator;
import org.chromium.chrome.tab_ui.R;
import org.chromium.components.browser_ui.bottomsheet.BottomSheetController;
import org.chromium.components.browser_ui.widget.scrim.ScrimCoordinator;
import org.chromium.ui.modaldialog.ModalDialogManager;
import org.chromium.ui.widget.ChromeImageView;

public class BraveTabGroupUiCoordinator extends TabGroupUiCoordinator {
    // To delete in bytecode, members from parent class will be used instead.
    private TabGroupUiToolbarView mToolbarView;

    // Own members.
    private TabModelSelector mTabModelSelector;

    public BraveTabGroupUiCoordinator(
            @NonNull Activity activity,
            @NonNull ViewGroup parentView,
            @NonNull BrowserControlsStateProvider browserControlsStateProvider,
            @NonNull ScrimCoordinator scrimCoordinator,
            @NonNull ObservableSupplier<Boolean> omniboxFocusStateSupplier,
            @NonNull BottomSheetController bottomSheetController,
            @NonNull DataSharingTabManager dataSharingTabManager,
            @NonNull TabModelSelector tabModelSelector,
            @NonNull TabContentManager tabContentManager,
            @NonNull TabCreatorManager tabCreatorManager,
            @NonNull OneshotSupplier<LayoutStateProvider> layoutStateProviderSupplier,
            @NonNull ModalDialogManager modalDialogManager,
            @NonNull ThemeColorProvider themeColorProvider) {
        super(
                activity,
                parentView,
                browserControlsStateProvider,
                scrimCoordinator,
                omniboxFocusStateSupplier,
                bottomSheetController,
                dataSharingTabManager,
                tabModelSelector,
                tabContentManager,
                tabCreatorManager,
                layoutStateProviderSupplier,
                modalDialogManager,
                themeColorProvider);

        mTabModelSelector = tabModelSelector;

        assert mToolbarView != null : "Make sure mToolbarView is properly patched in bytecode.";
        ChromeImageView fadingEdgeStart =
                mToolbarView.findViewById(R.id.tab_strip_fading_edge_start);
        assert fadingEdgeStart != null : "Something has changed in upstream.";
        if (fadingEdgeStart != null) {
            fadingEdgeStart.setVisibility(View.GONE);
        }
        ChromeImageView fadingEdgeEnd = mToolbarView.findViewById(R.id.tab_strip_fading_edge_end);
        assert fadingEdgeEnd != null : "Something has changed in upstream.";
        if (fadingEdgeEnd != null) {
            fadingEdgeEnd.setVisibility(View.GONE);
        }
        ChromeImageView toolbarNewTabButton =
                mToolbarView.findViewById(R.id.toolbar_new_tab_button);
        assert toolbarNewTabButton != null : "Something has changed in upstream.";
        if (toolbarNewTabButton != null) {
            toolbarNewTabButton.setImageResource(R.drawable.brave_new_group_tab);
        }
    }

    @Override
    public void initializeWithNative(
            Activity activity,
            BottomControlsCoordinator.BottomControlsVisibilityController visibilityController,
            Callback<Object> onModelTokenChange) {
        // Fix for the null object crash at TabGroupSyncFeatures.isTabGroupSyncEnabled
        // Stack:
        // at TabGroupSyncFeatures.isTabGroupSyncEnabled (TabGroupSyncFeatures.java:18)
        // at TabGroupUiMediator.<init> (TabGroupUiMediator.java:143)
        // at TabGroupUiCoordinator.initializeWithNative (TabGroupUiCoordinator.java:238)
        // at BraveTabGroupUiCoordinator.initializeWithNative (BraveTabGroupUiCoordinator.java:97)
        // at BottomControlsCoordinator.lambda$new$1 (BottomControlsCoordinator.java:148)
        //
        // TabModelSelector.getModel returns EmptyTabModel with null profile when the model is not
        // found.
        // Upstream does not have this issue because they don't use BottomControlsCoordinator
        if (mTabModelSelector.getModels().size() < 2
                || mTabModelSelector.getModel(false).getProfile() == null) {
            mTabModelSelector.addObserver(
                    new TabModelSelectorObserver() {
                        @Override
                        public void onChange() {
                            if (mTabModelSelector.getModels().size() >= 2
                                    && mTabModelSelector.getModel(false).getProfile() != null) {
                                callSuperInitializeWithNative(
                                        activity, visibilityController, onModelTokenChange);

                                mTabModelSelector.removeObserver(this);
                            }
                        }
                    });
        } else {
            callSuperInitializeWithNative(activity, visibilityController, onModelTokenChange);
        }
    }

    private void callSuperInitializeWithNative(
            Activity activity,
            BottomControlsCoordinator.BottomControlsVisibilityController visibilityController,
            Callback<Object> onModelTokenChange) {
        super.initializeWithNative(activity, visibilityController, onModelTokenChange);
    }

    @Override
    public void destroy() {
        try {
            super.destroy();
        } catch (NullPointerException ignore) {
            // mTabStripCoordinator could be null in a base class.
            // https://github.com/brave/brave-browser/issues/40673
        }
    }
}
