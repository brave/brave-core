/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tabbed_mode;

import static org.chromium.chrome.browser.lifecycle.ActivityLifecycleDispatcher.ActivityState.RESUMED_WITH_NATIVE;
import static org.chromium.chrome.browser.util.PictureInPictureUtils.deviceSupportedPictureInPictureMode;
import static org.chromium.chrome.browser.util.PictureInPictureUtils.hasPictureInPicturePermissionEnabled;
import static org.chromium.chrome.browser.util.PictureInPictureUtils.launchPictureInPictureSettings;

import android.app.PictureInPictureParams;
import android.os.Bundle;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

import org.chromium.base.Callback;
import org.chromium.base.Log;
import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.ObservableSupplierImpl;
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.base.supplier.OneshotSupplierImpl;
import org.chromium.base.supplier.Supplier;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ActivityTabProvider;
import org.chromium.chrome.browser.BackgroundVideoPlaybackTabHelper;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.back_press.BackPressManager;
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
import org.chromium.chrome.browser.layouts.LayoutType;
import org.chromium.chrome.browser.lifecycle.ActivityLifecycleDispatcher;
import org.chromium.chrome.browser.multiwindow.MultiInstanceManager;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.share.ShareDelegate;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabHidingType;
import org.chromium.chrome.browser.tab_ui.TabContentManager;
import org.chromium.chrome.browser.tab_ui.TabSwitcher;
import org.chromium.chrome.browser.tabmodel.TabCreatorManager;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.tabmodel.TabModelSelectorTabObserver;
import org.chromium.chrome.browser.toolbar.ToolbarIntentMetadata;
import org.chromium.chrome.browser.ui.appmenu.AppMenuBlocker;
import org.chromium.chrome.browser.ui.appmenu.AppMenuDelegate;
import org.chromium.chrome.browser.ui.edge_to_edge.EdgeToEdgeController;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager;
import org.chromium.chrome.browser.ui.system.StatusBarColorController.StatusBarColorProvider;
import org.chromium.components.browser_ui.edge_to_edge.EdgeToEdgeManager;
import org.chromium.components.browser_ui.edge_to_edge.SystemBarColorHelper;
import org.chromium.components.browser_ui.widget.MenuOrKeyboardActionController;
import org.chromium.content_public.browser.LoadUrlParams;
import org.chromium.content_public.browser.NavigationHandle;
import org.chromium.net.NetError;
import org.chromium.ui.InsetObserver;
import org.chromium.ui.base.ActivityWindowAndroid;
import org.chromium.ui.base.IntentRequestTracker;
import org.chromium.ui.modaldialog.ModalDialogManager;
import org.chromium.ui.widget.Toast;

import java.util.function.BooleanSupplier;
import java.util.function.Function;

public class BraveTabbedRootUiCoordinator extends TabbedRootUiCoordinator {
    private final AppCompatActivity mActivity;
    private final OneshotSupplier<HubManager> mHubManagerSupplier;
    private YouTubeFeaturesLayout mYouTubeLayout;

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
            @NonNull OneshotSupplierImpl<SystemBarColorHelper> systemBarColorHelperSupplier,
            @ActivityType int activityType,
            @NonNull Supplier<Boolean> isInOverviewModeSupplier,
            @NonNull AppMenuDelegate appMenuDelegate,
            @NonNull StatusBarColorProvider statusBarColorProvider,
            @NonNull OneshotSupplierImpl<EphemeralTabCoordinator> ephemeralTabCoordinatorSupplier,
            @NonNull IntentRequestTracker intentRequestTracker,
            @NonNull InsetObserver insetObserver,
            @NonNull Function<Tab, Boolean> backButtonShouldCloseTabFn,
            @NonNull Callback<Tab> sendToBackground,
            boolean initializeUiWithIncognitoColors,
            @NonNull BackPressManager backPressManager,
            @Nullable Bundle savedInstanceState,
            @Nullable MultiInstanceManager multiInstanceManager,
            @Nullable ObservableSupplier<Integer> overviewColorSupplier,
            @Nullable View baseChromeLayout,
            @NonNull ManualFillingComponentSupplier manualFillingComponentSupplier,
            @NonNull EdgeToEdgeManager edgeToEdgeManager) {
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
                baseChromeLayout,
                manualFillingComponentSupplier,
                edgeToEdgeManager);

        mActivity = activity;
        mHubManagerSupplier = hubManagerSupplier;
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

        ViewGroup viewGroup = mActivity.getWindow().getDecorView().findViewById(android.R.id.content);
        mYouTubeLayout = new YouTubeFeaturesLayout(mActivity, new YouTubeFeaturesLayout.Callback() {
            @Override
            public void onFullscreenClick() {
                final Tab currentTab = mTabModelSelectorSupplier.get().getCurrentTab();
                //noinspection ConstantValue
                if (TabUtils.isYouTubeVideo(currentTab) && mActivity instanceof BraveActivity braveActivity) {
                    if (braveActivity.isActivityFinishingOrDestroyed()) {
                        return;
                    }
                    BackgroundVideoPlaybackTabHelper.setFullscreen(currentTab.getWebContents());
                }
            }

            @Override
            public void onPictureInPictureClick() {
                if (!deviceSupportedPictureInPictureMode(mActivity)) {
                    Toast.makeText(mActivity, R.string.picture_in_picture_not_supported, Toast.LENGTH_LONG).show();
                    return;
                }

                if (!hasPictureInPicturePermissionEnabled(mActivity)) {
                    launchPictureInPictureSettings(mActivity);
                    return;
                }

                final Tab currentTab = mTabModelSelectorSupplier.get().getCurrentTab();
                //noinspection ConstantValue
                if (TabUtils.isYouTubeVideo(currentTab) && mActivity instanceof BraveActivity braveActivity) {
                    if (braveActivity.getLifecycleDispatcher().getCurrentActivityState() != RESUMED_WITH_NATIVE) {
                        return;
                    }
                    BackgroundVideoPlaybackTabHelper.setFullscreen(currentTab.getWebContents());
                    braveActivity.enterPictureInPictureMode(new PictureInPictureParams.Builder().build());
                }
            }
        });

        FrameLayout.LayoutParams params = new FrameLayout.LayoutParams(FrameLayout.LayoutParams.WRAP_CONTENT, FrameLayout.LayoutParams.WRAP_CONTENT);
        params.bottomMargin = (int) mActivity.getResources().getDimension(R.dimen.youtube_features_layout_margin_bottom);
        params.rightMargin = (int) mActivity.getResources().getDimension(R.dimen.youtube_features_layout_margin_right);
        params.gravity = Gravity.BOTTOM | Gravity.END;

        mYouTubeLayout.setLayoutParams(params);
        viewGroup.addView(mYouTubeLayout);

        mTabModelSelectorSupplier.addObserver(selector -> new TabModelSelectorTabObserver(selector) {
            @Override
            public void onShown(Tab tab, int type) {
                if (mIsInOverviewModeSupplier.get()) {
                    return;
                }

                mYouTubeLayout.setVisibility(tab);
            }

            @Override
            public void onDidFinishNavigationInPrimaryMainFrame(Tab tab, NavigationHandle navigation) {
                mYouTubeLayout.setVisibility(tab);
            }
        });
    }

    @Override
    protected void onLayoutManagerAvailable(LayoutManagerImpl layoutManager) {
        super.onLayoutManagerAvailable(layoutManager);

        mHubManagerSupplier.onAvailable(
                hubManager -> {
                    // Make it negative to indicate that we adjust the bottom margin.
                    int bottomToolbarHeight =
                            mActivity
                                            .getResources()
                                            .getDimensionPixelSize(R.dimen.bottom_controls_height)
                                    * -1;
                    hubManager.setStatusIndicatorHeight(bottomToolbarHeight);
                });

        layoutManager.addObserver(new LayoutStateProvider.LayoutStateObserver() {
            @Override
            public void onStartedShowing(int layoutType) {
                LayoutStateProvider.LayoutStateObserver.super.onStartedShowing(layoutType);
                showYouTubeExtraFeatures(true, layoutType, mTabModelSelectorSupplier.get());
            }

            @Override
            public void onStartedHiding(int layoutType) {
                LayoutStateProvider.LayoutStateObserver.super.onStartedHiding(layoutType);
                showYouTubeExtraFeatures(false, layoutType, mTabModelSelectorSupplier.get());
            }
        });

    }

    private void showYouTubeExtraFeatures(final boolean show,
                                          @LayoutType final int layoutType,
                                          @NonNull final TabModelSelector tabModelSelector) {
        if (layoutType != LayoutType.BROWSING || mYouTubeLayout == null) {
            return;
        }

        mYouTubeLayout.setVisibility(tabModelSelector.getCurrentTab(), show);
    }
}
