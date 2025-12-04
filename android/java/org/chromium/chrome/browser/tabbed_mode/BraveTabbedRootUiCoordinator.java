/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tabbed_mode;

import android.os.Bundle;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

import org.chromium.base.Callback;
import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.ObservableSupplierImpl;
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.base.supplier.OneshotSupplierImpl;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ActivityTabProvider;
import org.chromium.chrome.browser.ChromeInactivityTracker;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.back_press.BackPressManager;
import org.chromium.chrome.browser.bookmarks.BookmarkManagerOpener;
import org.chromium.chrome.browser.bookmarks.BookmarkManagerOpenerImpl;
import org.chromium.chrome.browser.bookmarks.BookmarkModel;
import org.chromium.chrome.browser.bookmarks.TabBookmarker;
import org.chromium.chrome.browser.compositor.CompositorViewHolder;
import org.chromium.chrome.browser.compositor.layouts.LayoutManagerImpl;
import org.chromium.chrome.browser.ephemeraltab.EphemeralTabCoordinator;
import org.chromium.chrome.browser.flags.ActivityType;
import org.chromium.chrome.browser.fullscreen.BrowserControlsManager;
import org.chromium.chrome.browser.fullscreen.FullscreenManager;
import org.chromium.chrome.browser.hub.HubManager;
import org.chromium.chrome.browser.keyboard_accessory.ManualFillingComponentSupplier;
import org.chromium.chrome.browser.layouts.LayoutStateProvider;
import org.chromium.chrome.browser.lifecycle.ActivityLifecycleDispatcher;
import org.chromium.chrome.browser.multiwindow.MultiInstanceManager;
import org.chromium.chrome.browser.ntp_customization.edge_to_edge.TopInsetCoordinator;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.share.ShareDelegate;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab_ui.TabContentManager;
import org.chromium.chrome.browser.tab_ui.TabSwitcher;
import org.chromium.chrome.browser.tabmodel.TabCreatorManager;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.toolbar.ToolbarHairlineView;
import org.chromium.chrome.browser.toolbar.ToolbarIntentMetadata;
import org.chromium.chrome.browser.ui.BraveAdaptiveToolbarUiCoordinator;
import org.chromium.chrome.browser.ui.appmenu.AppMenuBlocker;
import org.chromium.chrome.browser.ui.appmenu.AppMenuDelegate;
import org.chromium.chrome.browser.ui.browser_window.ChromeAndroidTask;
import org.chromium.chrome.browser.ui.edge_to_edge.EdgeToEdgeController;
import org.chromium.chrome.browser.ui.edge_to_edge.EdgeToEdgeUtils;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager;
import org.chromium.chrome.browser.ui.system.StatusBarColorController.StatusBarColorProvider;
import org.chromium.components.browser_ui.widget.MenuOrKeyboardActionController;
import org.chromium.ui.base.ActivityWindowAndroid;
import org.chromium.ui.base.IntentRequestTracker;
import org.chromium.ui.base.ViewUtils;
import org.chromium.ui.edge_to_edge.EdgeToEdgeManager;
import org.chromium.ui.edge_to_edge.SystemBarColorHelper;
import org.chromium.ui.insets.InsetObserver;
import org.chromium.ui.modaldialog.ModalDialogManager;

import java.util.function.BooleanSupplier;
import java.util.function.Function;
import java.util.function.Supplier;

public class BraveTabbedRootUiCoordinator extends TabbedRootUiCoordinator {
    private final AppCompatActivity mBraveActivity;
    private final OneshotSupplier<HubManager> mHubManagerSupplier;
    private final ObservableSupplier<EdgeToEdgeController> mBraveEdgeToEdgeControllerSupplier;

    public BraveTabbedRootUiCoordinator(
            @NonNull AppCompatActivity activity,
            @Nullable Callback<Boolean> onOmniboxFocusChangedListener,
            @NonNull ObservableSupplier<ShareDelegate> shareDelegateSupplier,
            @NonNull ActivityTabProvider tabProvider,
            @NonNull ObservableSupplier<Profile> profileSupplier,
            @NonNull ObservableSupplier<BookmarkModel> bookmarkModelSupplier,
            @NonNull ObservableSupplier<TabBookmarker> tabBookmarkerSupplier,
            @NonNull ObservableSupplier<TabModelSelector> tabModelSelectorSupplier,
            @NonNull OneshotSupplier<TabSwitcher> tabSwitcherSupplier,
            @NonNull OneshotSupplier<TabSwitcher> incognitoTabSwitcherSupplier,
            @NonNull OneshotSupplier<HubManager> hubManagerSupplier,
            @NonNull OneshotSupplier<ToolbarIntentMetadata> intentMetadataOneshotSupplier,
            @NonNull OneshotSupplier<LayoutStateProvider> layoutStateProviderOneshotSupplier,
            @NonNull BrowserControlsManager browserControlsManager,
            @NonNull ActivityWindowAndroid windowAndroid,
            @NonNull OneshotSupplier<ChromeAndroidTask> chromeAndroidTaskSupplier,
            @NonNull ActivityLifecycleDispatcher activityLifecycleDispatcher,
            @NonNull ObservableSupplier<LayoutManagerImpl> layoutManagerSupplier,
            @NonNull MenuOrKeyboardActionController menuOrKeyboardActionController,
            @NonNull Supplier<Integer> activityThemeColorSupplier,
            @NonNull ObservableSupplier<ModalDialogManager> modalDialogManagerSupplier,
            @NonNull AppMenuBlocker appMenuBlocker,
            @NonNull BooleanSupplier supportsAppMenuSupplier,
            @NonNull BooleanSupplier supportsFindInPage,
            @NonNull Supplier<TabCreatorManager> tabCreatorManagerSupplier,
            @NonNull FullscreenManager fullscreenManager,
            @NonNull Supplier<CompositorViewHolder> compositorViewHolderSupplier,
            @NonNull Supplier<TabContentManager> tabContentManagerSupplier,
            @NonNull Supplier<SnackbarManager> snackbarManagerSupplier,
            @NonNull ObservableSupplierImpl<EdgeToEdgeController> edgeToEdgeSupplier,
            @NonNull ObservableSupplierImpl<TopInsetCoordinator> topInsetCoordinatorSupplier,
            @NonNull OneshotSupplierImpl<SystemBarColorHelper> systemBarColorHelperSupplier,
            @ActivityType int activityType,
            @NonNull Supplier<Boolean> isInOverviewModeSupplier,
            @NonNull AppMenuDelegate appMenuDelegate,
            @NonNull StatusBarColorProvider statusBarColorProvider,
            @NonNull
                    ObservableSupplierImpl<EphemeralTabCoordinator> ephemeralTabCoordinatorSupplier,
            @NonNull IntentRequestTracker intentRequestTracker,
            @NonNull InsetObserver insetObserver,
            @NonNull Function<Tab, Boolean> backButtonShouldCloseTabFn,
            @NonNull Callback<Tab> sendToBackground,
            boolean initializeUiWithIncognitoColors,
            @NonNull BackPressManager backPressManager,
            @Nullable Bundle savedInstanceState,
            @Nullable MultiInstanceManager multiInstanceManager,
            @NonNull ObservableSupplier<Integer> overviewColorSupplier,
            @NonNull ManualFillingComponentSupplier manualFillingComponentSupplier,
            @NonNull EdgeToEdgeManager edgeToEdgeManager,
            @NonNull ObservableSupplier<BookmarkManagerOpener> bookmarkManagerOpenerSupplier,
            @Nullable ObservableSupplier<Boolean> xrSpaceModeObservableSupplier,
            @NonNull OneshotSupplier<ChromeInactivityTracker> inactivityTrackerSupplier) {
        super(
                activity,
                onOmniboxFocusChangedListener,
                shareDelegateSupplier,
                tabProvider,
                profileSupplier,
                bookmarkModelSupplier,
                tabBookmarkerSupplier,
                tabModelSelectorSupplier,
                tabSwitcherSupplier,
                incognitoTabSwitcherSupplier,
                hubManagerSupplier,
                intentMetadataOneshotSupplier,
                layoutStateProviderOneshotSupplier,
                browserControlsManager,
                windowAndroid,
                chromeAndroidTaskSupplier,
                activityLifecycleDispatcher,
                layoutManagerSupplier,
                menuOrKeyboardActionController,
                activityThemeColorSupplier,
                modalDialogManagerSupplier,
                appMenuBlocker,
                supportsAppMenuSupplier,
                supportsFindInPage,
                tabCreatorManagerSupplier,
                fullscreenManager,
                compositorViewHolderSupplier,
                tabContentManagerSupplier,
                snackbarManagerSupplier,
                edgeToEdgeSupplier,
                topInsetCoordinatorSupplier,
                systemBarColorHelperSupplier,
                activityType,
                isInOverviewModeSupplier,
                appMenuDelegate,
                statusBarColorProvider,
                ephemeralTabCoordinatorSupplier,
                intentRequestTracker,
                insetObserver,
                backButtonShouldCloseTabFn,
                sendToBackground,
                initializeUiWithIncognitoColors,
                backPressManager,
                savedInstanceState,
                multiInstanceManager,
                overviewColorSupplier,
                manualFillingComponentSupplier,
                edgeToEdgeManager,
                bookmarkManagerOpenerSupplier,
                xrSpaceModeObservableSupplier,
                inactivityTrackerSupplier);

        mBraveActivity = activity;
        mHubManagerSupplier = hubManagerSupplier;
        mBraveEdgeToEdgeControllerSupplier = edgeToEdgeSupplier;
    }

    @Override
    public void onPostInflationStartup() {
        super.onPostInflationStartup();

        assert mBraveActivity instanceof BraveActivity;

        if (mBraveActivity instanceof BraveActivity) {
            ((BraveActivity) mBraveActivity)
                    .updateBottomSheetPosition(
                            mBraveActivity.getResources().getConfiguration().orientation);
        }
    }

    @Override
    protected void onLayoutManagerAvailable(LayoutManagerImpl layoutManager) {
        super.onLayoutManagerAvailable(layoutManager);

        mHubManagerSupplier.onAvailable(
                hubManager -> {
                    // Make it negative to indicate that we adjust the bottom margin.
                    int bottomToolbarHeight =
                            mBraveActivity
                                            .getResources()
                                            .getDimensionPixelSize(R.dimen.bottom_controls_height)
                                    * -1;
                    if (EdgeToEdgeUtils.isEdgeToEdgeBottomChinEnabled(mBraveActivity)
                            && mBraveEdgeToEdgeControllerSupplier.get() != null) {
                        bottomToolbarHeight -=
                                mBraveEdgeToEdgeControllerSupplier.get().getBottomInsetPx();
                    }
                    hubManager.setStatusIndicatorHeight(bottomToolbarHeight);
                });
    }

    @Override
    protected void initializeToolbar() {
        // Needs to be called before super.initializeToolbar() to ensure the toolbar hairline is
        // initialized with the correct height.
        initializeToolbarHairline();

        super.initializeToolbar();

        assert mAdaptiveToolbarUiCoordinator instanceof BraveAdaptiveToolbarUiCoordinator
                : "Bytecode change was not applied!";
        if (mAdaptiveToolbarUiCoordinator
                instanceof BraveAdaptiveToolbarUiCoordinator braveCoordinator) {
            braveCoordinator.initializeBrave(new BookmarkManagerOpenerImpl());
        }
    }

    private void initializeToolbarHairline() {
        if (mBraveActivity != null) {
            final View controlContainer = mBraveActivity.findViewById(R.id.control_container);
            assert controlContainer != null : "Something has changed in the upstream code!";
            if (controlContainer != null) {
                final ToolbarHairlineView toolbarHairline =
                        controlContainer.findViewById(R.id.toolbar_hairline);
                assert toolbarHairline != null : "Something has changed in the upstream code!";
                if (toolbarHairline != null) {
                    // Make sure the height is set to the correct value to be used by
                    // getMeasuredHeight()
                    toolbarHairline.getLayoutParams().height =
                            mBraveActivity
                                    .getResources()
                                    .getDimensionPixelSize(R.dimen.toolbar_hairline_height);
                    ViewUtils.requestLayout(
                            toolbarHairline,
                            "BraveTabbedRootUiCoordinator.initializeToolbarHairline");
                }
            }
        }
    }
}
