/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.bottom;

import android.content.Context;
import android.content.res.Resources;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnLongClickListener;
import android.view.ViewGroup;
import android.view.ViewStub;

import org.chromium.base.Callback;
import org.chromium.base.CallbackController;
import org.chromium.base.ContextUtils;
import org.chromium.base.metrics.RecordUserAction;
import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.ObservableSupplierImpl;
import org.chromium.base.supplier.OneShotCallback;
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.base.supplier.Supplier;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ActivityTabProvider;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.ChromeActivity;
import org.chromium.chrome.browser.compositor.layouts.EmptyOverviewModeObserver;
import org.chromium.chrome.browser.compositor.layouts.OverviewModeBehavior;
import org.chromium.chrome.browser.compositor.layouts.OverviewModeBehavior.OverviewModeObserver;
import org.chromium.chrome.browser.feature_engagement.TrackerFactory;
import org.chromium.chrome.browser.homepage.HomepageManager;
import org.chromium.chrome.browser.omnibox.OmniboxFocusReason;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.share.ShareDelegate;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.IncognitoStateProvider;
import org.chromium.chrome.browser.tasks.ReturnToChromeExperimentsUtil;
import org.chromium.chrome.browser.toolbar.HomeButton;
import org.chromium.chrome.browser.toolbar.TabCountProvider;
import org.chromium.chrome.browser.toolbar.ThemeColorProvider;
import org.chromium.chrome.browser.toolbar.bottom.BookmarksButton;
import org.chromium.chrome.browser.toolbar.bottom.BottomToolbarNewTabButton;
import org.chromium.chrome.browser.toolbar.bottom.SearchAccelerator;
import org.chromium.chrome.browser.ui.appmenu.AppMenuButtonHelper;
import org.chromium.chrome.browser.util.TabUtils;
import org.chromium.components.feature_engagement.EventConstants;
import org.chromium.components.feature_engagement.Tracker;
import org.chromium.ui.widget.Toast;

/**
 * The root coordinator for the bottom toolbar. It has two sub-components: the browsing mode bottom
 * toolbar and the tab switcher mode bottom toolbar.
 */
class BottomToolbarCoordinator implements View.OnLongClickListener {
    /** The browsing mode bottom toolbar component */
    protected final BrowsingModeBottomToolbarCoordinator mBrowsingModeCoordinator;

    /** The tab switcher mode bottom toolbar component */
    private TabSwitcherBottomToolbarCoordinator mTabSwitcherModeCoordinator;

    /** The tab switcher mode bottom toolbar stub that will be inflated when native is ready. */
    private final ViewStub mTabSwitcherModeStub;

    /** A provider that notifies components when the theme color changes.*/
    private final ThemeColorProvider mThemeColorProvider;

    /** The overview mode manager. */
    protected OverviewModeBehavior mOverviewModeBehavior;
    protected OverviewModeObserver mOverviewModeObserver;

    /** The activity tab provider. */
    private ActivityTabProvider mTabProvider;

    private final ObservableSupplier<ShareDelegate> mShareDelegateSupplier;
    private final Callback<ShareDelegate> mShareDelegateSupplierCallback;
    private ObservableSupplierImpl<OnClickListener> mShareButtonListenerSupplier =
            new ObservableSupplierImpl<>();
    private final Supplier<Boolean> mShowStartSurfaceCallable;
    private CallbackController mCallbackController = new CallbackController();
    ObservableSupplier<AppMenuButtonHelper> mMenuButtonHelperSupplier;
    private BottomControlsMediator mBottomControlsMediator;
    private Runnable mOriginalHomeButtonRunnable;
    private final ScrollingBottomViewResourceFrameLayout mScrollingBottomView;
    private HomeButton mHomeButton;
    private BookmarksButton mBookmarksButton;
    private SearchAccelerator mSearchAccelerator;
    private BottomToolbarNewTabButton mNewTabButton;

    private final Context mContext = ContextUtils.getApplicationContext();

    /**
     * Build the coordinator that manages the bottom toolbar.
     * @param stub The bottom toolbar {@link ViewStub} to inflate.
     * @param tabProvider The {@link ActivityTabProvider} used for making the IPH.
     * @param themeColorProvider The {@link ThemeColorProvider} for the bottom toolbar.
     * @param shareDelegateSupplier The supplier for the {@link ShareDelegate} the bottom controls
     *         should use to share content.
     * @param showStartSurfaceCallable The action that opens the start surface, returning true if
     * the start surface is shown.
     * @param openHomepageAction The action that opens the homepage.
     * @param setUrlBarFocusAction The function that sets Url bar focus. The first argument is
     * @param overviewModeBehaviorSupplier Supplier for the overview mode manager.
     * @param menuButtonHelperSupplier
     */
    BottomToolbarCoordinator(ScrollingBottomViewResourceFrameLayout scrollingBottomView,
            ViewStub stub, ActivityTabProvider tabProvider,
            OnLongClickListener tabsSwitcherLongClickListner, ThemeColorProvider themeColorProvider,
            ObservableSupplier<ShareDelegate> shareDelegateSupplier,
            Supplier<Boolean> showStartSurfaceCallable, Runnable openHomepageAction,
            Callback<Integer> setUrlBarFocusAction,
            OneshotSupplier<OverviewModeBehavior> overviewModeBehaviorSupplier,
            ObservableSupplier<AppMenuButtonHelper> menuButtonHelperSupplier,
            BottomControlsMediator bottomControlsMediator) {
        View root = stub.inflate();

        overviewModeBehaviorSupplier.onAvailable(mCallbackController.makeCancelable(
                (overviewModeBehavior) -> { setOverviewModeBehavior(overviewModeBehavior); }));

        mShowStartSurfaceCallable = showStartSurfaceCallable;
        final OnClickListener homeButtonListener = v -> {
            openHomepageAction.run();
        };

        final OnClickListener searchAcceleratorListener = v -> {
            RecordUserAction.record("MobileToolbarOmniboxAcceleratorTap");

            // Only switch to HomePage when overview is showing.
            if (mOverviewModeBehavior != null && mOverviewModeBehavior.overviewVisible()
                    && mShowStartSurfaceCallable != null) {
                mShowStartSurfaceCallable.get();
            }
            setUrlBarFocusAction.onResult(OmniboxFocusReason.ACCELERATOR_TAP);
        };

        mBrowsingModeCoordinator = new BrowsingModeBottomToolbarCoordinator(root, tabProvider,
                homeButtonListener, searchAcceleratorListener, mShareButtonListenerSupplier,
                tabsSwitcherLongClickListner, overviewModeBehaviorSupplier);

        mTabSwitcherModeStub = root.findViewById(R.id.bottom_toolbar_tab_switcher_mode_stub);

        mThemeColorProvider = themeColorProvider;
        mTabProvider = tabProvider;

        mShareDelegateSupplier = shareDelegateSupplier;
        mShareDelegateSupplierCallback = this::onShareDelegateAvailable;
        mShareDelegateSupplier.addObserver(mShareDelegateSupplierCallback);

        mMenuButtonHelperSupplier = menuButtonHelperSupplier;
        mBottomControlsMediator = bottomControlsMediator;
        mOriginalHomeButtonRunnable = openHomepageAction;
        mScrollingBottomView = scrollingBottomView;
    }

    /**
     * Initialize the bottom toolbar with the components that had native initialization
     * dependencies.
     * <p>
     * Calling this must occur after the native library have completely loaded.
     * @param tabSwitcherListener An {@link OnClickListener} that is triggered when the
     *                            tab switcher button is clicked.
     * @param newTabClickListener An {@link OnClickListener} that is triggered when the
     *                            new tab button is clicked.
     * @param tabCountProvider Updates the tab count number in the tab switcher button and in the
     *                         incognito toggle tab layout.
     * @param incognitoStateProvider Notifies components when incognito mode is entered or exited.
     * @param topToolbarRoot The root {@link ViewGroup} of the top toolbar.
     * @param closeAllTabsAction The runnable that closes all tabs in the current tab model.
     */
    void initializeWithNative(OnClickListener tabSwitcherListener,
            OnClickListener newTabClickListener, TabCountProvider tabCountProvider,
            IncognitoStateProvider incognitoStateProvider, ViewGroup topToolbarRoot,
            Runnable closeAllTabsAction) {
        final OnClickListener closeTabsClickListener = v -> {
            final boolean isIncognito = incognitoStateProvider.isIncognitoSelected();
            if (isIncognito) {
                RecordUserAction.record("MobileToolbarCloseAllIncognitoTabsButtonTap");
            } else {
                RecordUserAction.record("MobileToolbarCloseAllRegularTabsButtonTap");
            }

            closeAllTabsAction.run();
        };

        mBrowsingModeCoordinator.initializeWithNative(newTabClickListener, tabSwitcherListener,
                mMenuButtonHelperSupplier, tabCountProvider, mThemeColorProvider,
                incognitoStateProvider);
        mTabSwitcherModeCoordinator = new TabSwitcherBottomToolbarCoordinator(mTabSwitcherModeStub,
                topToolbarRoot, incognitoStateProvider, mThemeColorProvider, newTabClickListener,
                closeTabsClickListener, mMenuButtonHelperSupplier, tabCountProvider);

        // Do not change bottom bar if StartSurface Single Pane is enabled and HomePage is not
        // customized.
        if (!ReturnToChromeExperimentsUtil.shouldShowStartSurfaceAsTheHomePage()
                && BottomToolbarVariationManager.shouldBottomToolbarBeVisibleInOverviewMode()) {
            mOverviewModeObserver = new EmptyOverviewModeObserver() {
                @Override
                public void onOverviewModeStartedShowing(boolean showToolbar) {
                    if (mBottomControlsMediator instanceof BraveBottomControlsMediator) {
                        ((BraveBottomControlsMediator) mBottomControlsMediator)
                                .setCompositedViewVisibile(false);
                    }
                    BrowsingModeBottomToolbarCoordinator browsingModeCoordinator =
                            (BrowsingModeBottomToolbarCoordinator) mBrowsingModeCoordinator;
                    browsingModeCoordinator.getSearchAccelerator().setVisibility(View.GONE);
                    if (BottomToolbarVariationManager.isShareButtonOnBottom()) {
                        browsingModeCoordinator.getShareButton().setVisibility(View.GONE);
                    }
                    if (BottomToolbarVariationManager.isHomeButtonOnBottom()) {
                        browsingModeCoordinator.getHomeButton().setVisibility(View.INVISIBLE);
                    }
                    if (BottomToolbarVariationManager.isBookmarkButtonOnBottom()) {
                        browsingModeCoordinator.getBookmarkButton().setVisibility(View.INVISIBLE);
                    }
                    if (BottomToolbarVariationManager.isTabSwitcherOnBottom()) {
                        browsingModeCoordinator.getTabSwitcherButtonView().setVisibility(
                                View.INVISIBLE);
                    }
                    if (BottomToolbarVariationManager.isNewTabButtonOnBottom()) {
                        browsingModeCoordinator.getNewTabButtonParent().setVisibility(View.VISIBLE);
                    }
                }

                @Override
                public void onOverviewModeStartedHiding(
                        boolean showToolbar, boolean delayAnimation) {
                    BrowsingModeBottomToolbarCoordinator browsingModeCoordinator =
                            (BrowsingModeBottomToolbarCoordinator) mBrowsingModeCoordinator;
                    browsingModeCoordinator.getSearchAccelerator().setVisibility(View.VISIBLE);
                    if (BottomToolbarVariationManager.isShareButtonOnBottom()) {
                        browsingModeCoordinator.getShareButton().setVisibility(View.VISIBLE);
                        browsingModeCoordinator.getShareButton().updateButtonEnabledState(
                                mTabProvider.get());
                    }
                    if (BottomToolbarVariationManager.isHomeButtonOnBottom()) {
                        browsingModeCoordinator.getHomeButton().setVisibility(View.VISIBLE);
                        browsingModeCoordinator.getHomeButton().updateButtonEnabledState(
                                mTabProvider.get());
                    }
                    if (BottomToolbarVariationManager.isBookmarkButtonOnBottom()) {
                        browsingModeCoordinator.getBookmarkButton().setVisibility(View.VISIBLE);
                    }
                    if (BottomToolbarVariationManager.isTabSwitcherOnBottom()) {
                        browsingModeCoordinator.getTabSwitcherButtonView().setVisibility(
                                View.VISIBLE);
                    }
                    if (BottomToolbarVariationManager.isNewTabButtonOnBottom()) {
                        browsingModeCoordinator.getNewTabButtonParent().setVisibility(View.GONE);
                    }
                }

                @Override
                public void onOverviewModeFinishedHiding() {
                    if (mBottomControlsMediator instanceof BraveBottomControlsMediator) {
                        ((BraveBottomControlsMediator) mBottomControlsMediator)
                                .setCompositedViewVisibile(true);
                    }
                }
            };
        }

        View root = (View) topToolbarRoot.getParent();
        View bottomToolbarBrowsing = root.findViewById(R.id.bottom_toolbar_browsing);
        View bottomToolbarButtons = root.findViewById(R.id.bottom_toolbar_buttons);

        mHomeButton = bottomToolbarBrowsing.findViewById(R.id.bottom_home_button);
        if (mHomeButton != null) {
            mHomeButton.setOnLongClickListener(this);

            final OnClickListener homeButtonListener = v -> {
                final boolean isHomepageEnabled = HomepageManager.isHomepageEnabled();
                if (isHomepageEnabled) {
                    mOriginalHomeButtonRunnable.run();
                } else {
                    newTabClickListener.onClick(v);
                }
            };

            mHomeButton.setOnClickListener(homeButtonListener);
        }

        mBookmarksButton = bottomToolbarBrowsing.findViewById(R.id.bottom_bookmark_button);
        if (mBookmarksButton != null) {
            mBookmarksButton.setOnLongClickListener(this);
        }

        mSearchAccelerator = bottomToolbarBrowsing.findViewById(R.id.search_accelerator);
        if (mSearchAccelerator != null) {
            mSearchAccelerator.setOnLongClickListener(this);
        }

        mNewTabButton = bottomToolbarButtons.findViewById(R.id.bottom_new_tab_button);
        if (mNewTabButton != null) {
            mNewTabButton.setOnLongClickListener(this);
        }

        ChromeActivity activity = BraveActivity.getBraveActivity();
        if (mScrollingBottomView != null && activity != null) {
            mScrollingBottomView.setSwipeDetector(
                    activity.getCompositorViewHolder().getLayoutManager().getToolbarSwipeHandler());
        }
    }

    /**
     * @param isVisible Whether the bottom toolbar is visible.
     */
    void setBottomToolbarVisible(boolean isVisible) {
        if (mTabSwitcherModeCoordinator != null) {
            mTabSwitcherModeCoordinator.showToolbarOnTop(!isVisible);
        }
        mBrowsingModeCoordinator.onVisibilityChanged(isVisible);
    }

    /**
     * Clean up any state when the bottom toolbar is destroyed.
     */
    void destroy() {
        mBrowsingModeCoordinator.destroy();
        if (mTabSwitcherModeCoordinator != null) {
            mTabSwitcherModeCoordinator.destroy();
            mTabSwitcherModeCoordinator = null;
        }
        if (mOverviewModeBehavior != null) {
            mOverviewModeBehavior.removeOverviewModeObserver(mOverviewModeObserver);
            mOverviewModeBehavior = null;
        }
        mThemeColorProvider.destroy();
        mShareDelegateSupplier.removeObserver(mShareDelegateSupplierCallback);
    }

    private void onShareDelegateAvailable(ShareDelegate shareDelegate) {
        final OnClickListener shareButtonListener = v -> {
            if (BottomToolbarVariationManager.isShareButtonOnBottom()) {
                RecordUserAction.record("MobileBottomToolbarShareButton");
            }

            Tab tab = mTabProvider.get();
            shareDelegate.share(tab, /*shareDirectly=*/false);
        };

        mShareButtonListenerSupplier.set(shareButtonListener);
    }

    private void setOverviewModeBehavior(OverviewModeBehavior overviewModeBehavior) {
        assert overviewModeBehavior != null;
        assert mOverviewModeBehavior
                == null
            : "TODO(https://crbug.com/1084528): the overview mode manager should set at most once.";
        mOverviewModeBehavior = overviewModeBehavior;
        mOverviewModeBehavior.addOverviewModeObserver(mOverviewModeObserver);
    }

    public void updateBookmarkButton(boolean isBookmarked, boolean editingAllowed) {
        if (mBrowsingModeCoordinator != null) {
            ((BrowsingModeBottomToolbarCoordinator) mBrowsingModeCoordinator)
                    .updateBookmarkButton(isBookmarked, editingAllowed);
        }
    }

    @Override
    public boolean onLongClick(View v) {
        String description = "";
        Resources resources = mContext.getResources();

        if (v == mHomeButton) {
            // It is currently a new tab button when homepage is disabled.
            if (!HomepageManager.isHomepageEnabled()) {
                TabUtils.showTabPopupMenu(mContext, v);
                return true;
            }

            description = resources.getString(R.string.accessibility_toolbar_btn_home);
        } else if (v == mBookmarksButton) {
            description = resources.getString(R.string.accessibility_toolbar_btn_bookmark);
        } else if (v == mSearchAccelerator) {
            description =
                    resources.getString(R.string.accessibility_toolbar_btn_search_accelerator);
        } else if (v == mNewTabButton) {
            TabUtils.showTabPopupMenu(mContext, v);
            return true;
        }

        return Toast.showAnchoredToast(mContext, v, description);
    }
}
