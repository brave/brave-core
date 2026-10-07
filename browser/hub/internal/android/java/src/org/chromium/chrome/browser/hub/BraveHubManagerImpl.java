/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.hub;

import android.app.Activity;
import android.content.ComponentCallbacks;
import android.content.res.Configuration;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.FrameLayout.LayoutParams;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.BraveReflectionUtil;
import org.chromium.base.supplier.MonotonicObservableSupplier;
import org.chromium.base.supplier.NonNullObservableSupplier;
import org.chromium.base.supplier.NullableObservableSupplier;
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.back_press.BackPressManager;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.profiles.ProfileProvider;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.toolbar.menu_button.MenuButtonCoordinator;
import org.chromium.chrome.browser.toolbar.settings.AddressBarPreference;
import org.chromium.chrome.browser.ui.bottombar.BottomBarConfigUtils;
import org.chromium.chrome.browser.ui.bottombar.BottomBarHostManager;
import org.chromium.chrome.browser.ui.edge_to_edge.EdgeToEdgeController;
import org.chromium.chrome.browser.ui.edge_to_edge.EdgeToEdgeControllerFactory;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager;
import org.chromium.chrome.browser.ui.searchactivityutils.SearchActivityClient;
import org.chromium.components.browser_ui.bottomsheet.BottomSheetController;
import org.chromium.components.browser_ui.widget.MenuOrKeyboardActionController;
import org.chromium.ui.base.DeviceFormFactor;
import org.chromium.ui.edge_to_edge.EdgeToEdgePadAdjuster;

/**
 * Brave's extension of {@link HubManagerImpl}. We need it to adjust bottom margin for the bottom
 * toolbar, when it is visible.
 */
public class BraveHubManagerImpl extends HubManagerImpl {
    private final Activity mActivity;
    private int mBottomToolbarHeight;
    private final boolean mIsTablet;
    private final ComponentCallbacks mComponentCallbacks;
    // This field is deleted by BraveHubManagerImplClassAdapter so the parent's field is used.
    private MonotonicObservableSupplier<EdgeToEdgeController> mEdgeToEdgeSupplier;
    private EdgeToEdgePadAdjuster mEdgeToEdgePadAdjuster;
    private @Nullable View mActionContainerWithLayoutListener;

    public BraveHubManagerImpl(
            Activity activity,
            OneshotSupplier<ProfileProvider> profileProviderSupplier,
            PaneListBuilder paneListBuilder,
            BackPressManager backPressManager,
            MenuOrKeyboardActionController menuOrKeyboardActionController,
            SnackbarManager snackbarManager,
            BottomSheetController bottomSheetController,
            @Nullable BottomBarHostManager bottomBarHostManager,
            NullableObservableSupplier<Tab> tabSupplier,
            MenuButtonCoordinator menuButtonCoordinator,
            HubShowPaneHelper hubShowPaneHelper,
            MonotonicObservableSupplier<EdgeToEdgeController> edgeToEdgeSupplier,
            SearchActivityClient searchActivityClient,
            NonNullObservableSupplier<Boolean> xrSpaceModeObservableSupplier,
            @PaneId int defaultPaneId) {
        super(
                activity,
                profileProviderSupplier,
                paneListBuilder,
                backPressManager,
                menuOrKeyboardActionController,
                snackbarManager,
                bottomSheetController,
                bottomBarHostManager,
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
        maybeRepositionToolbarToBottom();
    }

    @Override
    public void onHubLayoutDoneHiding() {
        // The parent destroys the HubCoordinator (and all views) here, so the adjuster's
        // view reference becomes stale. Reset it so it's recreated on the next show.
        if (mEdgeToEdgePadAdjuster != null) {
            mEdgeToEdgePadAdjuster.destroy();
            mEdgeToEdgePadAdjuster = null;
        }
        mActionContainerWithLayoutListener = null;
        super.onHubLayoutDoneHiding();
    }

    @Override
    public void destroy() {
        super.destroy();

        if (mActivity != null) {
            mActivity.unregisterComponentCallbacks(mComponentCallbacks);
        }
        if (mEdgeToEdgePadAdjuster != null) {
            mEdgeToEdgePadAdjuster.destroy();
            mEdgeToEdgePadAdjuster = null;
        }
    }

    private void maybeUpdateBottomMarginForContainerView() {
        if (mIsTablet || shouldShowToolbarAtBottom()) return;

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
                                    ChromeSharedPreferences.getInstance()
                                                    .readBoolean(
                                                            BravePreferenceKeys
                                                                    .BRAVE_IS_MENU_FROM_BOTTOM,
                                                            true)
                                            ? mBottomToolbarHeight
                                            : 0;
                            containerView.setLayoutParams(params);
                        });
    }

    /**
     * Repositions the hub toolbar from top to bottom when the address bar is at the bottom or the
     * bottom bar is enabled. This moves all hub controls (new tab, menu, pane switcher, search) to
     * the bottom of the tab switcher for one-handed accessibility.
     */
    private void maybeRepositionToolbarToBottom() {
        if (mIsTablet || !shouldShowToolbarAtBottom()) return;

        HubContainerView containerView = getContainerView();
        if (containerView == null) return;

        View hubToolbar = containerView.findViewById(R.id.hub_toolbar);
        if (hubToolbar == null) return;

        // Move the toolbar layout wrapper to the bottom of its FrameLayout parent.
        View toolbarWrapper = (View) hubToolbar.getParent();
        if (toolbarWrapper != null
                && toolbarWrapper.getLayoutParams() instanceof FrameLayout.LayoutParams) {
            FrameLayout.LayoutParams params =
                    (FrameLayout.LayoutParams) toolbarWrapper.getLayoutParams();
            params.gravity = Gravity.BOTTOM;
            toolbarWrapper.setLayoutParams(params);
        }

        swapActionButtonAndPaneSwitcher(hubToolbar);

        // Swap the pane host container margins: top margin → bottom margin so the content
        // area leaves space for the toolbar at the bottom instead of the top.
        View hostContainer = containerView.findViewById(R.id.hub_pane_host_container);
        if (hostContainer != null
                && hostContainer.getLayoutParams() instanceof FrameLayout.LayoutParams) {
            FrameLayout.LayoutParams params =
                    (FrameLayout.LayoutParams) hostContainer.getLayoutParams();
            if (params.topMargin > 0 && params.bottomMargin == 0) {
                int toolbarHeight = params.topMargin;
                params.topMargin = 0;
                params.bottomMargin = toolbarHeight;
                hostContainer.setLayoutParams(params);
            }
        }

        // Apply bottom padding for the system navigation bar so the toolbar content
        // does not sit behind the gesture bar, matching the address bar behavior.
        applyNavigationBarPadding(hubToolbar, hostContainer);
    }

    // Puts the new tab button in the center, which is easier to reach at the bottom.
    private void swapActionButtonAndPaneSwitcher(View hubToolbar) {
        View actionButton = hubToolbar.findViewById(R.id.toolbar_action_button);
        View paneSwitcherCard = hubToolbar.findViewById(R.id.pane_switcher_card);
        if (actionButton == null || paneSwitcherCard == null) return;

        View actionButtonGroup = (View) actionButton.getParent();
        if (!(actionButtonGroup.getLayoutParams() instanceof FrameLayout.LayoutParams)
                || !(paneSwitcherCard.getLayoutParams() instanceof FrameLayout.LayoutParams)) {
            return;
        }

        // The spacer aligns the button with the first tab, which would push it off center.
        View marginSpacer = actionButtonGroup.findViewById(R.id.margin_spacer);
        if (marginSpacer != null) marginSpacer.setVisibility(View.GONE);

        int edgeMargin =
                mActivity
                        .getResources()
                        .getDimensionPixelSize(R.dimen.hub_toolbar_action_button_start_margin);
        FrameLayout.LayoutParams paneSwitcherParams =
                (FrameLayout.LayoutParams) paneSwitcherCard.getLayoutParams();
        paneSwitcherParams.gravity = Gravity.START | Gravity.CENTER_VERTICAL;
        paneSwitcherParams.setMarginStart(edgeMargin);
        paneSwitcherCard.setLayoutParams(paneSwitcherParams);

        updateActionButtonPosition(actionButtonGroup, paneSwitcherCard, edgeMargin);

        // Pane count, button label and window width can all change after the first layout.
        View actionContainer = (View) actionButtonGroup.getParent();
        if (actionContainer == mActionContainerWithLayoutListener) return;
        mActionContainerWithLayoutListener = actionContainer;
        actionContainer.addOnLayoutChangeListener(
                (v, left, top, right, bottom, oldLeft, oldTop, oldRight, oldBottom) ->
                        updateActionButtonPosition(
                                actionButtonGroup, paneSwitcherCard, edgeMargin));
    }

    // Centers the new tab button, or moves it next to the end buttons if the pane switcher
    // would overlap it, e.g. on narrow windows.
    private void updateActionButtonPosition(
            View actionButtonGroup, View paneSwitcherCard, int minGap) {
        View actionContainer = (View) actionButtonGroup.getParent();
        int containerWidth = actionContainer.getWidth();

        int gravity = Gravity.CENTER;
        int marginEnd = 0;
        if (containerWidth > 0) {
            boolean isRtl = actionContainer.getLayoutDirection() == View.LAYOUT_DIRECTION_RTL;
            // The pane switcher is anchored at the start, so its end edge doesn't depend on
            // where the button currently is.
            int paneSwitcherEnd =
                    isRtl
                            ? containerWidth - paneSwitcherCard.getLeft()
                            : paneSwitcherCard.getRight();
            int centeredButtonStart = (containerWidth - actionButtonGroup.getWidth()) / 2;
            if (paneSwitcherEnd + minGap > centeredButtonStart) {
                gravity = Gravity.END | Gravity.CENTER_VERTICAL;
                View endButtons = actionContainer.findViewById(R.id.menu_button_container);
                if (endButtons != null && endButtons.getVisibility() != View.GONE) {
                    marginEnd = endButtons.getWidth();
                    // Match the gap between the shred and menu buttons.
                    View shredButton =
                            endButtons.findViewById(
                                    org.chromium.chrome.browser.brave_shields.R.id
                                            .shred_data_button);
                    if (shredButton != null
                            && shredButton.getLayoutParams()
                                    instanceof ViewGroup.MarginLayoutParams) {
                        marginEnd +=
                                ((ViewGroup.MarginLayoutParams) shredButton.getLayoutParams())
                                        .getMarginEnd();
                    }
                }
            }
        }

        FrameLayout.LayoutParams params =
                (FrameLayout.LayoutParams) actionButtonGroup.getLayoutParams();
        if (params.gravity == gravity && params.getMarginEnd() == marginEnd) return;
        params.gravity = gravity;
        params.setMarginEnd(marginEnd);
        actionButtonGroup.setLayoutParams(params);
    }

    /**
     * Applies bottom padding to the toolbar for the system navigation bar inset using the
     * edge-to-edge controller (same pattern as upstream HubBottomToolbarCoordinator), and adjusts
     * the pane host container's bottom margin when the toolbar wrapper's layout changes.
     */
    private void applyNavigationBarPadding(View hubToolbar, View hostContainer) {
        // Guard against multiple calls (onHubLayoutShow fires each time hub is shown).
        if (mEdgeToEdgePadAdjuster != null) return;

        View toolbarWrapper = (View) hubToolbar.getParent();
        if (toolbarWrapper == null) return;

        // Use the EdgeToEdge factory to apply bottom padding for the navigation bar inset.
        mEdgeToEdgePadAdjuster =
                EdgeToEdgeControllerFactory.createForViewAndObserveSupplier(
                        toolbarWrapper, mEdgeToEdgeSupplier);

        // When the toolbar wrapper's height changes (due to edge-to-edge padding), update
        // the pane host container's bottom margin to prevent content overlap.
        toolbarWrapper.addOnLayoutChangeListener(
                (v, left, top, right, bottom, oldLeft, oldTop, oldRight, oldBottom) -> {
                    int newHeight = bottom - top;
                    if (hostContainer != null
                            && hostContainer.getLayoutParams()
                                    instanceof FrameLayout.LayoutParams) {
                        FrameLayout.LayoutParams hostParams =
                                (FrameLayout.LayoutParams) hostContainer.getLayoutParams();
                        if (hostParams.bottomMargin != newHeight) {
                            hostParams.bottomMargin = newHeight;
                            hostContainer.setLayoutParams(hostParams);
                        }
                    }
                });
    }

    // The bottom bar is not shown in the hub, so its controls take its place at the bottom.
    private boolean shouldShowToolbarAtBottom() {
        return !AddressBarPreference.isToolbarConfiguredToShowOnTop()
                || BottomBarConfigUtils.isBottomBarEnabled(mActivity);
    }
}
