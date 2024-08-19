/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tasks.tab_management;

import android.content.Context;
import android.content.SharedPreferences;
import android.view.View.OnClickListener;

import androidx.annotation.NonNull;

import org.chromium.base.BraveReflectionUtil;
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.base.supplier.Supplier;
import org.chromium.chrome.browser.hub.HubContainerView;
import org.chromium.chrome.browser.hub.HubLayoutAnimatorProvider;
import org.chromium.chrome.browser.profiles.ProfileProvider;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.TabModelFilter;
import org.chromium.chrome.browser.tasks.tab_management.TabListCoordinator.TabListMode;
import org.chromium.chrome.browser.user_education.UserEducationHelper;

import java.util.function.DoubleConsumer;

public class BraveTabSwitcherPane extends TabSwitcherPane {

    public BraveTabSwitcherPane(
            @NonNull Context context,
            @NonNull SharedPreferences sharedPreferences,
            @NonNull OneshotSupplier<ProfileProvider> profileProviderSupplier,
            @NonNull TabSwitcherPaneCoordinatorFactory factory,
            @NonNull Supplier<TabModelFilter> tabModelFilterSupplier,
            @NonNull OnClickListener newTabButtonClickListener,
            @NonNull TabSwitcherPaneDrawableCoordinator tabSwitcherDrawableCoordinator,
            @NonNull DoubleConsumer onToolbarAlphaChange,
            @NonNull UserEducationHelper userEducationHelper) {
        super(
                context,
                sharedPreferences,
                profileProviderSupplier,
                factory,
                tabModelFilterSupplier,
                newTabButtonClickListener,
                tabSwitcherDrawableCoordinator,
                onToolbarAlphaChange,
                userEducationHelper);
    }

    @Override
    public @NonNull HubLayoutAnimatorProvider createHideHubLayoutAnimatorProvider(
            @NonNull HubContainerView hubContainerView) {
        int tabId = getCurrentTabId();
        if (getTabListMode() != TabListMode.LIST && tabId != Tab.INVALID_TAB_ID) {
            // Force call TabSwitcherPaneBase.createTabSwitcherPaneCoordinator
            // to ensure TabSwitcherPaneBase.mTabSwitcherPaneCoordinatorSupplier is set
            BraveReflectionUtil.InvokeMethod(
                    TabSwitcherPaneBase.class, this, "createTabSwitcherPaneCoordinator");
        }
        return super.createHideHubLayoutAnimatorProvider(hubContainerView);
    }
}
