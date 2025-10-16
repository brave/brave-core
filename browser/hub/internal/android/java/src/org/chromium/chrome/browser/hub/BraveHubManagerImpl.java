/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.hub;

import android.app.Activity;
import android.content.ComponentCallbacks;
import android.content.res.Configuration;
import android.widget.FrameLayout.LayoutParams;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.BraveReflectionUtil;
import org.chromium.base.ContextUtils;
import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.back_press.BackPressManager;
import org.chromium.chrome.browser.profiles.ProfileProvider;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.toolbar.menu_button.MenuButtonCoordinator;
import org.chromium.chrome.browser.toolbar.settings.AddressBarPreference;
import org.chromium.chrome.browser.ui.edge_to_edge.EdgeToEdgeController;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager;
import org.chromium.chrome.browser.ui.searchactivityutils.SearchActivityClient;
import org.chromium.components.browser_ui.widget.MenuOrKeyboardActionController;
import org.chromium.ui.base.DeviceFormFactor;

/**
 * Brave's extension of {@link HubManagerImpl}. We need it to adjust bottom margin for the bottom
 * toolbar, when it is visible.
 */
public class BraveHubManagerImpl extends HubManagerImpl {
    private final Activity mActivity;
    private int mBottomToolbarHeight;
    private final boolean mIsTablet;
    private final ComponentCallbacks mComponentCallbacks;

    public BraveHubManagerImpl(
            Activity activity,
            OneshotSupplier<ProfileProvider> profileProviderSupplier,
            PaneListBuilder paneListBuilder,
            BackPressManager backPressManager,
            MenuOrKeyboardActionController menuOrKeyboardActionController,
            SnackbarManager snackbarManager,
            ObservableSupplier<Tab> tabSupplier,
            MenuButtonCoordinator menuButtonCoordinator,
            HubShowPaneHelper hubShowPaneHelper,
            ObservableSupplier<EdgeToEdgeController> edgeToEdgeSupplier,
            SearchActivityClient searchActivityClient,
            @Nullable ObservableSupplier<Boolean> xrSpaceModeObservableSupplier,
            @PaneId int defaultPaneId) {
        super(
                activity,
                profileProviderSupplier,
                paneListBuilder,
                backPressManager,
                menuOrKeyboardActionController,
                snackbarManager,
                tabSupplier,
                menuButtonCoordinator,
                hubShowPaneHelper,
                edgeToEdgeSupplier,
                searchActivityClient,
                xrSpaceModeObservableSupplier,
                defaultPaneId);

        mIsTablet = DeviceFormFactor.isNonMultiDisplayContextOnTablet(activity);

        mActivity = activity;
        mComponentCallbacks =
                new ComponentCallbacks() {
                    @Override
                    public void onConfigurationChanged(Configuration newConfig) {
                        if (mActivity != null
                                && (mActivity.isFinishing() || mActivity.isDestroyed())) {
                            return;
                        }
                        maybeUpdateBottomMarginForContainerView();
                    }

                    @Override
                    public void onLowMemory() {}
                };
        mActivity.registerComponentCallbacks(mComponentCallbacks);
    }

    @Override
    public void setStatusIndicatorHeight(int height) {
        // Tablets do not require this adjustment, so just ignore this case.
        if (mIsTablet && height < 0) return;

        // Negative height indicates that this is value for the bottom margin.
        if (height < 0) {
            mBottomToolbarHeight = height * -1;

            maybeUpdateBottomMarginForContainerView();
            return;
        }
        super.setStatusIndicatorHeight(height);
    }

    @Override
    public void onHubLayoutShow() {
        super.onHubLayoutShow();

        maybeUpdateBottomMarginForContainerView();
    }

    @Override
    public void destroy() {
        super.destroy();

        mActivity.unregisterComponentCallbacks(mComponentCallbacks);
    }

    private void maybeUpdateBottomMarginForContainerView() {
        if (mIsTablet || isToolbarBottomAnchored()) return;

        // We want to prevent crash at cr136
        // which happened at:
        //      browser.hub.HubToolbarMediator$1.onConfigurationChanged
        //      browser.hub.HubToolbarMediator.<init>
        //      browser.hub.HubToolbarCoordinator.<init>
        //      browser.hub.HubCoordinator.<init>
        //      browser.hub.HubManagerImpl.ensureHubCoordinatorIsInitialized
        //      java.lang.reflect.Method.invoke
        //      base.BraveReflectionUtil.invokeMethod
        //      browser.hub.BraveHubManagerImpl.maybeUpdateBottomMarginForContainerView
        // because mPaneManager.getFocusedPaneSupplier().get() was null
        // We can just wait on the supplier till the object will be ready.

        // If Pane is already set, ObservableSupplier.addSyncObserverAndCallIfNonNull
        // will call it immediately. removeObserver is also invoked somehow according to the logs.
        getPaneManager()
                .getFocusedPaneSupplier()
                .addSyncObserverAndCallIfNonNull(
                        (pane_unused) -> {
                            BraveReflectionUtil.invokeMethod(
                                    HubManagerImpl.class,
                                    this,
                                    "ensureHubCoordinatorIsInitialized");

                            HubContainerView containerView = getContainerView();
                            LayoutParams params = (LayoutParams) containerView.getLayoutParams();
                            params.bottomMargin =
                                    ContextUtils.getAppSharedPreferences()
                                                    .getBoolean(
                                                            BravePreferenceKeys
                                                                    .BRAVE_IS_MENU_FROM_BOTTOM,
                                                            true)
                                            ? mBottomToolbarHeight
                                            : 0;
                            containerView.setLayoutParams(params);
                        });
    }

    private boolean isToolbarBottomAnchored() {
        return !AddressBarPreference.isToolbarConfiguredToShowOnTop();
    }
}
