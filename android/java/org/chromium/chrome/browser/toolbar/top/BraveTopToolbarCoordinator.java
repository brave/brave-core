/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.top;

import android.content.Context;
import android.view.View;
import android.view.View.OnLongClickListener;

import androidx.annotation.ColorInt;

import org.chromium.base.BraveReflectionUtil;
import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.base.supplier.Supplier;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.browser_controls.BrowserStateBrowserControlsVisibilityDelegate;
import org.chromium.chrome.browser.fullscreen.FullscreenManager;
import org.chromium.chrome.browser.layouts.LayoutStateProvider;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabObscuringHandler;
import org.chromium.chrome.browser.theme.ThemeColorProvider;
import org.chromium.chrome.browser.toolbar.ToolbarDataProvider;
import org.chromium.chrome.browser.toolbar.ToolbarProgressBar;
import org.chromium.chrome.browser.toolbar.ToolbarTabController;
import org.chromium.chrome.browser.toolbar.back_button.BackButtonCoordinator;
import org.chromium.chrome.browser.toolbar.menu_button.MenuButton;
import org.chromium.chrome.browser.toolbar.menu_button.MenuButtonCoordinator;
import org.chromium.chrome.browser.toolbar.optional_button.ButtonDataProvider;
import org.chromium.chrome.browser.toolbar.top.NavigationPopup.HistoryDelegate;
import org.chromium.chrome.browser.toolbar.top.tab_strip.TabStripTransitionCoordinator.TabStripTransitionDelegate;
import org.chromium.chrome.browser.ui.appmenu.AppMenuButtonHelper;
import org.chromium.chrome.browser.user_education.UserEducationHelper;
import org.chromium.components.browser_ui.desktop_windowing.DesktopWindowStateManager;
import org.chromium.ui.resources.ResourceManager;
import org.chromium.ui.util.ColorUtils;

import java.util.List;

public class BraveTopToolbarCoordinator extends TopToolbarCoordinator {
    // To delete in bytecode. Variables from the parent class will be used instead.
    private OptionalBrowsingModeButtonController mOptionalButtonController;

    // Own members.
    private final ToolbarLayout mBraveToolbarLayout;
    private final MenuButtonCoordinator mBraveMenuButtonCoordinator;
    private boolean mIsBottomControlsVisible;
    private final ObservableSupplier<Integer> mConstraintsProxy;
    private final ToolbarControlContainer mControlContainer;
    private boolean mInTabSwitcherMode;

    public BraveTopToolbarCoordinator(
            ToolbarControlContainer controlContainer,
            ToolbarLayout toolbarLayout,
            ToolbarDataProvider toolbarDataProvider,
            ToolbarTabController tabController,
            UserEducationHelper userEducationHelper,
            List<ButtonDataProvider> buttonDataProviders,
            OneshotSupplier<LayoutStateProvider> layoutStateProviderSupplier,
            ThemeColorProvider normalThemeColorProvider,
            MenuButtonCoordinator browsingModeMenuButtonCoordinator,
            ObservableSupplier<AppMenuButtonHelper> appMenuButtonHelperSupplier,
            ToggleTabStackButtonCoordinator tabSwitcherButtonCoordinator,
            ObservableSupplier<Integer> tabCountSupplier,
            ObservableSupplier<Boolean> homepageEnabledSupplier,
            ObservableSupplier<Boolean> homepageNonNtpSupplier,
            Supplier<ResourceManager> resourceManagerSupplier,
            HistoryDelegate historyDelegate,
            boolean initializeWithIncognitoColors,
            ObservableSupplier<Integer> constraintsSupplier,
            ObservableSupplier<Boolean> compositorInMotionSupplier,
            BrowserStateBrowserControlsVisibilityDelegate
                    browserStateBrowserControlsVisibilityDelegate,
            FullscreenManager fullscreenManager,
            TabObscuringHandler tabObscuringHandler,
            @Nullable DesktopWindowStateManager desktopWindowStateManager,
            OneshotSupplier<TabStripTransitionDelegate> tabStripTransitionDelegateSupplier,
            @Nullable OnLongClickListener onLongClickListener,
            ToolbarProgressBar progressBar,
            ObservableSupplier<@Nullable Tab> tabSupplier,
            ObservableSupplier<Boolean> toolbarNavControlsEnabledSupplier,
            @Nullable BackButtonCoordinator backButtonCoordinator,
            HomeButtonDisplay homeButtonDisplay) {
        super(
                controlContainer,
                toolbarLayout,
                toolbarDataProvider,
                tabController,
                userEducationHelper,
                buttonDataProviders,
                layoutStateProviderSupplier,
                normalThemeColorProvider,
                browsingModeMenuButtonCoordinator,
                appMenuButtonHelperSupplier,
                tabSwitcherButtonCoordinator,
                tabCountSupplier,
                homepageEnabledSupplier,
                homepageNonNtpSupplier,
                resourceManagerSupplier,
                historyDelegate,
                initializeWithIncognitoColors,
                constraintsSupplier,
                compositorInMotionSupplier,
                browserStateBrowserControlsVisibilityDelegate,
                fullscreenManager,
                tabObscuringHandler,
                desktopWindowStateManager,
                tabStripTransitionDelegateSupplier,
                onLongClickListener,
                progressBar,
                tabSupplier,
                toolbarNavControlsEnabledSupplier,
                backButtonCoordinator,
                homeButtonDisplay);

        mBraveToolbarLayout = toolbarLayout;
        mBraveMenuButtonCoordinator = browsingModeMenuButtonCoordinator;
        mConstraintsProxy = constraintsSupplier;
        mControlContainer = controlContainer;

        if (isToolbarPhone()) {
            // We basically do here what we must do at ToolbarPhone.ctor
            // mLocationBarBackgroundColorForNtp =
            //      getContext().getColor(R.color.location_bar_background_color_for_ntp);
            // But we can't use bytecode patching to overide constructor because ToolbarPhone object
            // is created via reflection during inflate of layout/toolbar_phone.xml.
            // So use reflection to set ToolbarPhone.mLocationBarBackgroundColorForNtp

            // ContextUtils.getApplicationContext() does not respect Dark theme,
            // we must get ToolbarPhone context.
            Object toolbarContext =
                    BraveReflectionUtil.invokeMethod(
                            android.view.View.class, mBraveToolbarLayout, "getContext");

            assert toolbarContext instanceof Context;

            @ColorInt
            int locationBarBackgroundColorForNtp =
                    ((Context) toolbarContext)
                            .getColor(R.color.location_bar_background_color_for_ntp);

            BraveReflectionUtil.setField(
                    ToolbarPhone.class,
                    "mLocationBarBackgroundColorForNtp",
                    mBraveToolbarLayout,
                    locationBarBackgroundColorForNtp);

            // We need to set toolbar background color which in upstream is calculated
            // at ToolbarPhone.updateLocationBarLayoutForExpansionAnimation with
            // ColorUtils.blendColorsMultiply(...), which otherwise will be a bit
            // more gray than white and will have poor contrast with address bar area.
            @ColorInt
            int toolbarBackgroundColorForNtp =
                    ((Context) toolbarContext).getColor(R.color.toolbar_background_color_for_ntp);

            if (!ColorUtils.inNightMode((Context) toolbarContext)) {
                BraveReflectionUtil.setField(
                        ToolbarPhone.class,
                        "mToolbarBackgroundColorForNtp",
                        mBraveToolbarLayout,
                        toolbarBackgroundColorForNtp);
            }
        }
    }

    public void onBottomControlsVisibilityChanged(boolean isVisible) {
        mIsBottomControlsVisible = isVisible;
        if (mBraveToolbarLayout instanceof BraveToolbarLayout) {
            ((BraveToolbarLayoutImpl) mBraveToolbarLayout)
                    .onBottomControlsVisibilityChanged(isVisible);
        }
        if (mOptionalButtonController != null) {
            mOptionalButtonController.updateButtonVisibility();
        }
    }

    public boolean isToolbarPhone() {
        return mBraveToolbarLayout instanceof ToolbarPhone;
    }

    @Override
    public MenuButton getMenuButtonWrapper() {
        // We consider that there is no top toolbar menu button, if bottom controls are visible.
        return mIsBottomControlsVisible ? null : mBraveMenuButtonCoordinator.getMenuButton();
    }

    public ObservableSupplier<Integer> getConstraintsProxy() {
        return mConstraintsProxy;
    }

    @Override
    public void setTabSwitcherMode(boolean inTabSwitcherMode) {
        mInTabSwitcherMode = inTabSwitcherMode;

        super.setTabSwitcherMode(inTabSwitcherMode);
    }

    @Override
    public void onTabSwitcherTransitionFinished() {
        super.onTabSwitcherTransitionFinished();

        if (isToolbarPhone() && mInTabSwitcherMode) {
            // Make sure we have proper state at the end of the tab switcher transition.
            mControlContainer.setVisibility(View.VISIBLE);
        }
    }
}
