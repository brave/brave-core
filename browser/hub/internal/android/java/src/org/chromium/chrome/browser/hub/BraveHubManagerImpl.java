/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.hub;

import android.content.ComponentCallbacks;
import android.content.Context;
import android.content.res.Configuration;
import android.widget.FrameLayout.LayoutParams;

import androidx.annotation.NonNull;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.BraveReflectionUtil;
import org.chromium.base.ContextUtils;
import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.chrome.browser.back_press.BackPressManager;
import org.chromium.chrome.browser.profiles.ProfileProvider;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.toolbar.menu_button.MenuButtonCoordinator;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager;
import org.chromium.components.browser_ui.widget.MenuOrKeyboardActionController;
import org.chromium.ui.base.DeviceFormFactor;

/**
 * Brave's extension of {@link HubManagerImpl}. We need it to adjust bottom margin for the bottom
 * toolbar, when it is visible.
 */
public class BraveHubManagerImpl extends HubManagerImpl {
    private Context mContext;
    private int mBottomToolbarHeight;
    private boolean mBottomToolbarVisible;
    private boolean mIsTablet;
    private ComponentCallbacks mComponentCallbacks;

    public BraveHubManagerImpl(
            @NonNull Context context,
            @NonNull OneshotSupplier<ProfileProvider> profileProviderSupplier,
            @NonNull PaneListBuilder paneListBuilder,
            @NonNull BackPressManager backPressManager,
            @NonNull MenuOrKeyboardActionController menuOrKeyboardActionController,
            @NonNull SnackbarManager snackbarManager,
            @NonNull ObservableSupplier<Tab> tabSupplier,
            @NonNull MenuButtonCoordinator menuButtonCoordinator,
            @NonNull HubShowPaneHelper hubShowPaneHelper) {
        super(
                context,
                profileProviderSupplier,
                paneListBuilder,
                backPressManager,
                menuOrKeyboardActionController,
                snackbarManager,
                tabSupplier,
                menuButtonCoordinator,
                hubShowPaneHelper);

        mIsTablet = DeviceFormFactor.isNonMultiDisplayContextOnTablet(context);

        mContext = context;
        mComponentCallbacks =
                new ComponentCallbacks() {
                    @Override
                    public void onConfigurationChanged(Configuration newConfig) {
                        maybeUpdateBottomMarginForContainerView();
                    }

                    @Override
                    public void onLowMemory() {}
                };
        mContext.registerComponentCallbacks(mComponentCallbacks);
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

        mContext.unregisterComponentCallbacks(mComponentCallbacks);
    }

    private void maybeUpdateBottomMarginForContainerView() {
        if (mIsTablet) return;

        BraveReflectionUtil.invokeMethod(
                HubManagerImpl.class, this, "ensureHubCoordinatorIsInitialized");

        HubContainerView containerView = getContainerView();
        LayoutParams params = (LayoutParams) containerView.getLayoutParams();
        params.bottomMargin =
                ContextUtils.getAppSharedPreferences()
                                .getBoolean(BravePreferenceKeys.BRAVE_IS_MENU_FROM_BOTTOM, true)
                        ? mBottomToolbarHeight
                        : 0;
        containerView.setLayoutParams(params);
    }
}
