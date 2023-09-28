/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar;

import android.content.res.Configuration;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewStub;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

import org.chromium.base.Callback;
import org.chromium.base.CallbackController;
import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.ObservableSupplierImpl;
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.base.supplier.Supplier;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ActivityTabProvider;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.ChromeActivity;
import org.chromium.chrome.browser.app.tab_activity_glue.TabReparentingController;
import org.chromium.chrome.browser.back_press.BackPressManager;
import org.chromium.chrome.browser.bookmarks.BookmarkModel;
import org.chromium.chrome.browser.browser_controls.BrowserControlsSizer;
import org.chromium.chrome.browser.compositor.CompositorViewHolder;
import org.chromium.chrome.browser.compositor.bottombar.ephemeraltab.EphemeralTabCoordinator;
import org.chromium.chrome.browser.compositor.layouts.LayoutManagerImpl;
import org.chromium.chrome.browser.compositor.layouts.content.TabContentManager;
import org.chromium.chrome.browser.findinpage.FindToolbarManager;
import org.chromium.chrome.browser.fullscreen.FullscreenManager;
import org.chromium.chrome.browser.homepage.HomepageManager;
import org.chromium.chrome.browser.identity_disc.IdentityDiscController;
import org.chromium.chrome.browser.layouts.LayoutStateProvider;
import org.chromium.chrome.browser.lifecycle.ActivityLifecycleDispatcher;
import org.chromium.chrome.browser.merchant_viewer.MerchantTrustSignalsCoordinator;
import org.chromium.chrome.browser.omnibox.LocationBar;
import org.chromium.chrome.browser.omnibox.suggestions.history_clusters.HistoryClustersProcessor.OpenHistoryClustersDelegate;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.share.ShareDelegate;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabObscuringHandler;
import org.chromium.chrome.browser.tabmodel.IncognitoStateProvider;
import org.chromium.chrome.browser.tabmodel.TabCreatorManager;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.tasks.tab_management.TabGroupUi;
import org.chromium.chrome.browser.tasks.tab_management.TabManagementDelegateProvider;
import org.chromium.chrome.browser.theme.TopUiThemeColorProvider;
import org.chromium.chrome.browser.toolbar.bottom.BottomControlsCoordinator;
import org.chromium.chrome.browser.toolbar.bottom.BottomToolbarConfiguration;
import org.chromium.chrome.browser.toolbar.bottom.BraveBottomControlsCoordinator;
import org.chromium.chrome.browser.toolbar.bottom.BraveScrollingBottomViewResourceFrameLayout;
import org.chromium.chrome.browser.toolbar.menu_button.BraveMenuButtonCoordinator;
import org.chromium.chrome.browser.toolbar.menu_button.MenuButtonCoordinator;
import org.chromium.chrome.browser.toolbar.top.ActionModeController;
import org.chromium.chrome.browser.toolbar.top.BottomTabSwitcherActionMenuCoordinator;
import org.chromium.chrome.browser.toolbar.top.BraveTopToolbarCoordinator;
import org.chromium.chrome.browser.toolbar.top.ToolbarActionModeCallback;
import org.chromium.chrome.browser.toolbar.top.ToolbarControlContainer;
import org.chromium.chrome.browser.toolbar.top.TopToolbarCoordinator;
import org.chromium.chrome.browser.ui.appmenu.AppMenuCoordinator;
import org.chromium.chrome.browser.ui.appmenu.AppMenuDelegate;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager;
import org.chromium.chrome.browser.ui.system.StatusBarColorController;
import org.chromium.chrome.features.start_surface.StartSurface;
import org.chromium.components.browser_ui.bottomsheet.BottomSheetController;
import org.chromium.components.browser_ui.widget.scrim.ScrimCoordinator;
import org.chromium.components.omnibox.action.OmniboxActionDelegate;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.modaldialog.ModalDialogManager;

import java.util.List;

public class BraveToolbarManager extends ToolbarManager {
    private static final String TAG = "BraveToolbarManager";

    // To delete in bytecode, members from parent class will be used instead.
    private ObservableSupplierImpl<BottomControlsCoordinator> mBottomControlsCoordinatorSupplier;
    private CallbackController mCallbackController;
    private BrowserControlsSizer mBrowserControlsSizer;
    private FullscreenManager mFullscreenManager;
    private ActivityTabProvider mActivityTabProvider;
    private AppThemeColorProvider mAppThemeColorProvider;
    private ScrimCoordinator mScrimCoordinator;
    private Supplier<Boolean> mShowStartSurfaceSupplier;
    private MenuButtonCoordinator mMenuButtonCoordinator;
    private ToolbarTabControllerImpl mToolbarTabController;
    private LocationBar mLocationBar;
    private ActionModeController mActionModeController;
    private LocationBarModel mLocationBarModel;
    private TopToolbarCoordinator mToolbar;
    private ObservableSupplier<BookmarkModel> mBookmarkModelSupplier;
    private LayoutManagerImpl mLayoutManager;
    private ObservableSupplierImpl<Boolean> mOverlayPanelVisibilitySupplier;
    private TabModelSelector mTabModelSelector;
    private IncognitoStateProvider mIncognitoStateProvider;
    private TabCountProvider mTabCountProvider;
    private TabGroupUi mTabGroupUi;
    private BottomSheetController mBottomSheetController;
    private ActivityLifecycleDispatcher mActivityLifecycleDispatcher;
    private Supplier<Boolean> mIsWarmOnResumeSupplier;
    private TabContentManager mTabContentManager;
    private TabCreatorManager mTabCreatorManager;
    private SnackbarManager mSnackbarManager;
    private TabObscuringHandler mTabObscuringHandler;
    private LayoutStateProvider.LayoutStateObserver mLayoutStateObserver;
    private LayoutStateProvider mLayoutStateProvider;

    // Own members.
    private boolean mIsBottomToolbarVisible;
    private ObservableSupplier<Boolean> mOmniboxFocusStateSupplier;
    private OneshotSupplier<LayoutStateProvider> mLayoutStateProviderSupplier;
    private HomepageManager.HomepageStateListener mBraveHomepageStateListener;
    private AppCompatActivity mActivity;
    private WindowAndroid mWindowAndroid;
    private CompositorViewHolder mCompositorViewHolder;
    private final Object mLock = new Object();
    private boolean mBottomControlsEnabled;
    private BraveScrollingBottomViewResourceFrameLayout mBottomControls;

    public BraveToolbarManager(AppCompatActivity activity, BrowserControlsSizer controlsSizer,
            FullscreenManager fullscreenManager, ToolbarControlContainer controlContainer,
            CompositorViewHolder compositorViewHolder, Callback<Boolean> urlFocusChangedCallback,
            TopUiThemeColorProvider topUiThemeColorProvider,
            TabObscuringHandler tabObscuringHandler,
            ObservableSupplier<ShareDelegate> shareDelegateSupplier,
            IdentityDiscController identityDiscController,
            List<ButtonDataProvider> buttonDataProviders, ActivityTabProvider tabProvider,
            ScrimCoordinator scrimCoordinator, ToolbarActionModeCallback toolbarActionModeCallback,
            FindToolbarManager findToolbarManager, ObservableSupplier<Profile> profileSupplier,
            ObservableSupplier<BookmarkModel> bookmarkModelSupplier,
            @Nullable Supplier<Boolean> canAnimateNativeBrowserControls,
            OneshotSupplier<LayoutStateProvider> layoutStateProviderSupplier,
            OneshotSupplier<AppMenuCoordinator> appMenuCoordinatorSupplier,
            boolean shouldShowUpdateBadge,
            ObservableSupplier<TabModelSelector> tabModelSelectorSupplier,
            OneshotSupplier<StartSurface> startSurfaceSupplier,
            ObservableSupplier<Boolean> omniboxFocusStateSupplier,
            OneshotSupplier<Boolean> promoShownOneshotSupplier, WindowAndroid windowAndroid,
            Supplier<Boolean> isInOverviewModeSupplier,
            Supplier<ModalDialogManager> modalDialogManagerSupplier,
            StatusBarColorController statusBarColorController, AppMenuDelegate appMenuDelegate,
            ActivityLifecycleDispatcher activityLifecycleDispatcher,
            @NonNull Supplier<Tab> startSurfaceParentTabSupplier,
            @NonNull BottomSheetController bottomSheetController,
            @NonNull Supplier<Boolean> isWarmOnResumeSupplier,
            @NonNull TabContentManager tabContentManager,
            @NonNull TabCreatorManager tabCreatorManager, @NonNull SnackbarManager snackbarManager,
            @NonNull Supplier<MerchantTrustSignalsCoordinator>
                    merchantTrustSignalsCoordinatorSupplier,
            OneshotSupplier<TabReparentingController> tabReparentingControllerSupplier,
            @NonNull OmniboxActionDelegate omniboxActionDelegate,
            Supplier<EphemeralTabCoordinator> ephemeralTabCoordinatorSupplier,
            boolean initializeWithIncognitoColors, @Nullable BackPressManager backPressManager,
            @NonNull OpenHistoryClustersDelegate openHistoryClustersDelegate) {
        super(activity, controlsSizer, fullscreenManager, controlContainer, compositorViewHolder,
                urlFocusChangedCallback, topUiThemeColorProvider, tabObscuringHandler,
                shareDelegateSupplier, identityDiscController, buttonDataProviders, tabProvider,
                scrimCoordinator, toolbarActionModeCallback, findToolbarManager, profileSupplier,
                bookmarkModelSupplier, canAnimateNativeBrowserControls, layoutStateProviderSupplier,
                appMenuCoordinatorSupplier, shouldShowUpdateBadge, tabModelSelectorSupplier,
                startSurfaceSupplier, omniboxFocusStateSupplier, promoShownOneshotSupplier,
                windowAndroid, isInOverviewModeSupplier, modalDialogManagerSupplier,
                statusBarColorController, appMenuDelegate, activityLifecycleDispatcher,
                startSurfaceParentTabSupplier, bottomSheetController, isWarmOnResumeSupplier,
                tabContentManager, tabCreatorManager, snackbarManager,
                merchantTrustSignalsCoordinatorSupplier, tabReparentingControllerSupplier,
                omniboxActionDelegate, ephemeralTabCoordinatorSupplier,
                initializeWithIncognitoColors, backPressManager, openHistoryClustersDelegate);

        mOmniboxFocusStateSupplier = omniboxFocusStateSupplier;
        mLayoutStateProviderSupplier = layoutStateProviderSupplier;
        mActivity = activity;
        mWindowAndroid = windowAndroid;
        mCompositorViewHolder = compositorViewHolder;

        if (isToolbarPhone()) {
            updateBottomToolbarVisibility();
        }

        mBraveHomepageStateListener = () -> {
            if (mBottomControlsCoordinatorSupplier != null
                    && mBottomControlsCoordinatorSupplier.get()
                                    instanceof BraveBottomControlsCoordinator) {
                ((BraveBottomControlsCoordinator) mBottomControlsCoordinatorSupplier.get())
                        .updateHomeButtonState();
            }
        };
        HomepageManager.getInstance().addListener(mBraveHomepageStateListener);
    }

    @Override
    public void enableBottomControls() {
        assert (mActivity instanceof ChromeActivity);
        synchronized (mLock) {
            if (mBottomControlsEnabled) {
                return;
            }
            mBottomControlsEnabled = true;
            if (!BottomToolbarConfiguration.isBottomToolbarEnabled()) {
                super.enableBottomControls();
                return;
            }
            ViewStub bottomControlsStub =
                    (ViewStub) mActivity.findViewById(R.id.bottom_controls_stub);
            mBottomControls =
                    (BraveScrollingBottomViewResourceFrameLayout) bottomControlsStub.inflate();
            mTabGroupUi = TabManagementDelegateProvider.getDelegate().createTabGroupUi(mActivity,
                    mBottomControls.findViewById(R.id.bottom_container_slot), mBrowserControlsSizer,
                    mIncognitoStateProvider, mScrimCoordinator, mOmniboxFocusStateSupplier,
                    mBottomSheetController, mActivityLifecycleDispatcher, mIsWarmOnResumeSupplier,
                    mTabModelSelector, mTabContentManager, mCompositorViewHolder,
                    mCompositorViewHolder::getDynamicResourceLoader, mTabCreatorManager,
                    mLayoutStateProviderSupplier, mSnackbarManager);
            mBottomControlsCoordinatorSupplier.set(new BraveBottomControlsCoordinator(
                    mLayoutStateProviderSupplier,
                    BottomTabSwitcherActionMenuCoordinator.createOnLongClickListener(
                            id -> ((ChromeActivity) mActivity).onOptionsItemSelected(id, null)),
                    mActivityTabProvider, mToolbarTabController::openHomepage,
                    mCallbackController.makeCancelable((reason) -> setUrlBarFocus(true, reason)),
                    mMenuButtonCoordinator.getMenuButtonHelperSupplier(), mAppThemeColorProvider,
                    /* Below are parameters for BottomControlsCoordinator */
                    mActivity, mWindowAndroid, mLayoutManager,
                    mCompositorViewHolder.getResourceManager(), mBrowserControlsSizer,
                    mFullscreenManager, mBottomControls, mTabGroupUi, mTabObscuringHandler,
                    mOverlayPanelVisibilitySupplier, getConstraintsProxy(), mBookmarkModelSupplier,
                    mLocationBarModel));
            mBottomControls.setBottomControlsCoordinatorSupplier(
                    mBottomControlsCoordinatorSupplier);
            updateBottomToolbarVisibility();
            if (mIsBottomToolbarVisible) {
                mBottomControls.setVisibility(View.VISIBLE);
            }
        }
    }

    @Override
    public void initializeWithNative(LayoutManagerImpl layoutManager,
            OnClickListener tabSwitcherClickHandler, OnClickListener newTabClickHandler,
            OnClickListener bookmarkClickHandler, OnClickListener customTabsBackClickHandler,
            Supplier<Boolean> showStartSurfaceSupplier) {
        super.initializeWithNative(layoutManager, tabSwitcherClickHandler, newTabClickHandler,
                bookmarkClickHandler, customTabsBackClickHandler, showStartSurfaceSupplier);

        if (isToolbarPhone() && BottomToolbarConfiguration.isBottomToolbarEnabled()) {
            enableBottomControls();
            Runnable closeAllTabsAction = () -> {
                mTabModelSelector.getModel(mIncognitoStateProvider.isIncognitoSelected())
                        .closeAllTabs();
            };
            assert (mBottomControlsCoordinatorSupplier.get()
                            instanceof BraveBottomControlsCoordinator);
            ((BraveBottomControlsCoordinator) mBottomControlsCoordinatorSupplier.get())
                    .initializeWithNative(mActivity, mCompositorViewHolder.getResourceManager(),
                            mCompositorViewHolder.getLayoutManager(), tabSwitcherClickHandler,
                            newTabClickHandler, mWindowAndroid, mTabCountProvider,
                            mIncognitoStateProvider, mActivity.findViewById(R.id.control_container),
                            closeAllTabsAction);
            mLocationBar.getContainerView().setAccessibilityTraversalBefore(R.id.bottom_toolbar);
        }
    }

    @Override
    public @Nullable View getMenuButtonView() {
        if (mMenuButtonCoordinator.getMenuButton() == null) {
            // Return fake view instead of null to avoid NullPointerException as some code within
            // Chromium doesn't check for null.
            return new View(mActivity);
        }
        return super.getMenuButtonView();
    }

    @Override
    public void destroy() {
        super.destroy();
        HomepageManager.getInstance().removeListener(mBraveHomepageStateListener);
        if (mLayoutStateProvider != null) {
            mLayoutStateProvider.removeObserver(mLayoutStateObserver);
            mLayoutStateProvider = null;
        }
    }

    protected void onOrientationChange(int newOrientation) {
        if (mActionModeController != null) mActionModeController.showControlsOnOrientationChange();

        if (mBottomControlsCoordinatorSupplier.get() != null
                && BottomToolbarConfiguration.isBottomToolbarEnabled()) {
            boolean isBottomToolbarVisible = newOrientation != Configuration.ORIENTATION_LANDSCAPE;
            setBottomToolbarVisible(isBottomToolbarVisible);
        }

        assert mActivity instanceof BraveActivity;

        if (mActivity instanceof BraveActivity) {
            ((BraveActivity) mActivity).updateBottomSheetPosition(newOrientation);
        }
    }

    protected void updateBookmarkButtonStatus() {
        if (mBookmarkModelSupplier == null) return;
        Tab currentTab = mLocationBarModel.getTab();
        BookmarkModel bridge = mBookmarkModelSupplier.get();
        boolean isBookmarked =
                currentTab != null && bridge != null && bridge.hasBookmarkIdForTab(currentTab);
        boolean editingAllowed =
                currentTab == null || bridge == null || bridge.isEditBookmarksEnabled();
        mToolbar.updateBookmarkButton(isBookmarked, editingAllowed);

        if (mBottomControlsCoordinatorSupplier.get() instanceof BraveBottomControlsCoordinator) {
            ((BraveBottomControlsCoordinator) mBottomControlsCoordinatorSupplier.get())
                    .updateBookmarkButton(isBookmarked, editingAllowed);
        }
    }

    protected void updateReloadState(boolean tabCrashed) {
        assert (false);
    }

    private void setBottomToolbarVisible(boolean visible) {
        mIsBottomToolbarVisible = visible;
        boolean isMenuFromBottom =
                mIsBottomToolbarVisible && BottomToolbarConfiguration.isBottomToolbarEnabled();
        BraveMenuButtonCoordinator.setMenuFromBottom(isMenuFromBottom);
        if (mToolbar instanceof BraveTopToolbarCoordinator) {
            ((BraveTopToolbarCoordinator) mToolbar).onBottomToolbarVisibilityChanged(visible);
        }
        if (mBottomControlsCoordinatorSupplier.get() instanceof BraveBottomControlsCoordinator) {
            ((BraveBottomControlsCoordinator) mBottomControlsCoordinatorSupplier.get())
                    .setBottomToolbarVisible(visible);
        }
    }

    private void updateBottomToolbarVisibility() {
        boolean isBottomToolbarVisible = BottomToolbarConfiguration.isBottomToolbarEnabled()
                && mActivity.getResources().getConfiguration().orientation
                        != Configuration.ORIENTATION_LANDSCAPE;
        setBottomToolbarVisible(isBottomToolbarVisible);
    }

    private boolean isToolbarPhone() {
        assert (mToolbar instanceof BraveTopToolbarCoordinator);
        return mToolbar instanceof BraveTopToolbarCoordinator
                && ((BraveTopToolbarCoordinator) mToolbar).isToolbarPhone();
    }

    private ObservableSupplier<Integer> getConstraintsProxy() {
        if (mToolbar instanceof BraveTopToolbarCoordinator) {
            return ((BraveTopToolbarCoordinator) mToolbar).getConstraintsProxy();
        }

        assert false : "Wrong top toolbar type!";
        return null;
    }
}
