/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tabbed_mode;

import android.os.Bundle;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

import org.chromium.base.Callback;
import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.ObservableSupplierImpl;
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.base.supplier.Supplier;
import org.chromium.chrome.browser.ActivityTabProvider;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.tab_activity_glue.TabReparentingController;
import org.chromium.chrome.browser.back_press.BackPressManager;
import org.chromium.chrome.browser.bookmarks.BookmarkModel;
import org.chromium.chrome.browser.bookmarks.TabBookmarker;
import org.chromium.chrome.browser.compositor.CompositorViewHolder;
import org.chromium.chrome.browser.compositor.bottombar.ephemeraltab.EphemeralTabCoordinator;
import org.chromium.chrome.browser.compositor.layouts.LayoutManagerImpl;
import org.chromium.chrome.browser.compositor.layouts.content.TabContentManager;
import org.chromium.chrome.browser.contextualsearch.ContextualSearchManager;
import org.chromium.chrome.browser.flags.ActivityType;
import org.chromium.chrome.browser.fullscreen.BrowserControlsManager;
import org.chromium.chrome.browser.fullscreen.FullscreenManager;
import org.chromium.chrome.browser.layouts.LayoutStateProvider;
import org.chromium.chrome.browser.lifecycle.ActivityLifecycleDispatcher;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.share.ShareDelegate;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.TabCreatorManager;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.tasks.tab_management.TabSwitcher;
import org.chromium.chrome.browser.toolbar.ToolbarIntentMetadata;
import org.chromium.chrome.browser.ui.appmenu.AppMenuBlocker;
import org.chromium.chrome.browser.ui.appmenu.AppMenuDelegate;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager;
import org.chromium.chrome.browser.ui.system.StatusBarColorController.StatusBarColorProvider;
import org.chromium.chrome.features.start_surface.StartSurface;
import org.chromium.components.browser_ui.widget.InsetObserverView;
import org.chromium.components.browser_ui.widget.MenuOrKeyboardActionController;
import org.chromium.ui.base.ActivityWindowAndroid;
import org.chromium.ui.base.IntentRequestTracker;
import org.chromium.ui.modaldialog.ModalDialogManager;

import java.util.function.BooleanSupplier;
import java.util.function.Function;

public class BraveTabbedRootUiCoordinator extends TabbedRootUiCoordinator {
    protected AppCompatActivity mActivity;

    public BraveTabbedRootUiCoordinator(@NonNull AppCompatActivity activity,
            @Nullable Callback<Boolean> onOmniboxFocusChangedListener,
            @NonNull ObservableSupplier<ShareDelegate> shareDelegateSupplier,
            @NonNull ActivityTabProvider tabProvider,
            @NonNull ObservableSupplier<Profile> profileSupplier,
            @NonNull ObservableSupplier<BookmarkModel> bookmarkModelSupplier,
            @NonNull ObservableSupplier<TabBookmarker> tabBookmarkerSupplier,
            @NonNull Supplier<ContextualSearchManager> contextualSearchManagerSupplier,
            @NonNull ObservableSupplier<TabModelSelector> tabModelSelectorSupplier,
            @NonNull OneshotSupplier<StartSurface> startSurfaceSupplier,
            @NonNull OneshotSupplier<TabSwitcher> tabSwitcherSupplier,
            @NonNull OneshotSupplier<ToolbarIntentMetadata> intentMetadataOneshotSupplier,
            @NonNull OneshotSupplier<LayoutStateProvider> layoutStateProviderOneshotSupplier,
            @NonNull Supplier<Tab> startSurfaceParentTabSupplier,
            @NonNull BrowserControlsManager browserControlsManager,
            @NonNull ActivityWindowAndroid windowAndroid,
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
            @ActivityType int activityType, @NonNull Supplier<Boolean> isInOverviewModeSupplier,
            @NonNull Supplier<Boolean> isWarmOnResumeSupplier,
            @NonNull AppMenuDelegate appMenuDelegate,
            @NonNull StatusBarColorProvider statusBarColorProvider,
            @NonNull ObservableSupplierImpl<EphemeralTabCoordinator>
                    ephemeralTabCoordinatorSupplier,
            @NonNull IntentRequestTracker intentRequestTracker, int controlContainerHeightResource,
            @NonNull Supplier<InsetObserverView> insetObserverViewSupplier,
            @NonNull Function<Tab, Boolean> backButtonShouldCloseTabFn,
            OneshotSupplier<TabReparentingController> tabReparentingControllerSupplier,
            boolean initializeUiWithIncognitoColors, @NonNull BackPressManager backPressManager,
            @Nullable Bundle savedInstanceState) {
        super(activity, onOmniboxFocusChangedListener, shareDelegateSupplier, tabProvider,
                profileSupplier, bookmarkModelSupplier, tabBookmarkerSupplier,
                contextualSearchManagerSupplier, tabModelSelectorSupplier, startSurfaceSupplier,
                tabSwitcherSupplier, intentMetadataOneshotSupplier,
                layoutStateProviderOneshotSupplier, startSurfaceParentTabSupplier,
                browserControlsManager, windowAndroid, activityLifecycleDispatcher,
                layoutManagerSupplier, menuOrKeyboardActionController, activityThemeColorSupplier,
                modalDialogManagerSupplier, appMenuBlocker, supportsAppMenuSupplier,
                supportsFindInPage, tabCreatorManagerSupplier, fullscreenManager,
                compositorViewHolderSupplier, tabContentManagerSupplier, snackbarManagerSupplier,
                activityType, isInOverviewModeSupplier, isWarmOnResumeSupplier, appMenuDelegate,
                statusBarColorProvider, ephemeralTabCoordinatorSupplier, intentRequestTracker,
                controlContainerHeightResource, insetObserverViewSupplier,
                backButtonShouldCloseTabFn, tabReparentingControllerSupplier,
                initializeUiWithIncognitoColors, backPressManager, savedInstanceState);

        mActivity = activity;
    }

    @Override
    public void onPostInflationStartup() {
        super.onPostInflationStartup();

        assert mActivity instanceof BraveActivity;

        if (mActivity instanceof BraveActivity) {
            ((BraveActivity) mActivity)
                    .updateBottomSheetPosition(
                            mActivity.getResources().getConfiguration().orientation);
        }
    }
}
