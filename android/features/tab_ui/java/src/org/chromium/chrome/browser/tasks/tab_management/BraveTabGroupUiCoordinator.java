/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tasks.tab_management;

import static org.chromium.chrome.browser.tasks.tab_management.TabManagementModuleProvider.SYNTHETIC_TRIAL_POSTFIX;

import android.app.Activity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;

import androidx.annotation.NonNull;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.base.supplier.Supplier;
import org.chromium.chrome.browser.compositor.layouts.OverviewModeBehavior;
import org.chromium.chrome.browser.compositor.layouts.content.TabContentManager;
import org.chromium.chrome.browser.lifecycle.ActivityLifecycleDispatcher;
import org.chromium.chrome.browser.share.ShareDelegate;
import org.chromium.chrome.browser.tabmodel.IncognitoStateProvider;
import org.chromium.chrome.browser.tabmodel.IncognitoStateProvider.IncognitoStateObserver;
import org.chromium.chrome.browser.tabmodel.TabCreatorManager;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.toolbar.bottom.BottomControlsCoordinator;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager;
import org.chromium.chrome.tab_ui.R;
import org.chromium.components.browser_ui.bottomsheet.BottomSheetController;
import org.chromium.components.browser_ui.widget.scrim.ScrimCoordinator;
import org.chromium.ui.resources.dynamics.DynamicResourceLoader;
import org.chromium.ui.widget.ChromeImageView;

public class BraveTabGroupUiCoordinator extends TabGroupUiCoordinator {
    // To delete in bytecode, members from parent class will be used instead.
    private TabGroupUiToolbarView mToolbarView;

    // Own members.
    private IncognitoStateProvider mIncognitoStateProvider;
    private IncognitoStateObserver mIncognitoStateObserver;

    public BraveTabGroupUiCoordinator(@NonNull Activity activity, @NonNull ViewGroup parentView,
            @NonNull IncognitoStateProvider incognitoStateProvider,
            @NonNull ScrimCoordinator scrimCoordinator,
            @NonNull ObservableSupplier<Boolean> omniboxFocusStateSupplier,
            @NonNull BottomSheetController bottomSheetController,
            @NonNull ActivityLifecycleDispatcher activityLifecycleDispatcher,
            @NonNull Supplier<Boolean> isWarmOnResumeSupplier,
            @NonNull TabModelSelector tabModelSelector,
            @NonNull TabContentManager tabContentManager, @NonNull ViewGroup rootView,
            @NonNull Supplier<DynamicResourceLoader> dynamicResourceLoaderSupplier,
            @NonNull TabCreatorManager tabCreatorManager,
            @NonNull Supplier<ShareDelegate> shareDelegateSupplier,
            @NonNull OneshotSupplier<OverviewModeBehavior> overviewModeBehaviorSupplier,
            @NonNull SnackbarManager snackbarManager) {
        super(activity, parentView, incognitoStateProvider, scrimCoordinator,
                omniboxFocusStateSupplier, bottomSheetController, activityLifecycleDispatcher,
                isWarmOnResumeSupplier, tabModelSelector, tabContentManager, rootView,
                dynamicResourceLoaderSupplier, tabCreatorManager, shareDelegateSupplier,
                overviewModeBehaviorSupplier, snackbarManager);

        mIncognitoStateProvider = incognitoStateProvider;

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
        ChromeImageView toolbarRightButton = mToolbarView.findViewById(R.id.toolbar_right_button);
        assert toolbarRightButton != null : "Something has changed in upstream.";
        if (toolbarRightButton != null) {
            toolbarRightButton.setImageResource(R.drawable.brave_new_group_tab);
        }
    }

    @Override
    public void initializeWithNative(Activity activity,
            BottomControlsCoordinator.BottomControlsVisibilityController visibilityController) {
        super.initializeWithNative(activity, visibilityController);

        mIncognitoStateObserver = (isIncognito) -> {
            if (!isIncognito) {
                // Make sure that background color match bottom toolbar color.
                LinearLayout mainContent = mToolbarView.findViewById(R.id.main_content);
                assert mainContent != null : "Something has changed in upstream!";
                if (mainContent != null) {
                    mainContent.setBackgroundColor(
                            activity.getResources().getColor(R.color.dialog_bg_color_baseline));
                }
            }
        };
        mIncognitoStateProvider.addIncognitoStateObserverAndTrigger(mIncognitoStateObserver);
    }

    @Override
    public void destroy() {
        super.destroy();

        mIncognitoStateProvider.removeObserver(mIncognitoStateObserver);
    }
}
