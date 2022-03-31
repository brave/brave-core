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

import androidx.core.content.ContextCompat;

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
import org.chromium.chrome.browser.compositor.CompositorViewHolder;
import org.chromium.chrome.browser.feature_engagement.TrackerFactory;
import org.chromium.chrome.browser.homepage.HomepageManager;
import org.chromium.chrome.browser.layouts.LayoutStateProvider;
import org.chromium.chrome.browser.layouts.LayoutType;
import org.chromium.chrome.browser.omnibox.OmniboxFocusReason;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.IncognitoStateProvider;
import org.chromium.chrome.browser.tasks.ReturnToChromeExperimentsUtil;
import org.chromium.chrome.browser.tasks.tab_management.TabUiFeatureUtilities;
import org.chromium.chrome.browser.theme.ThemeColorProvider;
import org.chromium.chrome.browser.toolbar.HomeButton;
import org.chromium.chrome.browser.toolbar.TabCountProvider;
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

    private LayoutStateProvider.LayoutStateObserver mLayoutStateObserver;
    private OneshotSupplier<LayoutStateProvider> mLayoutStateProviderSupplier;
    private LayoutStateProvider mLayoutStateProvider;

    /** The activity tab provider. */
    private ActivityTabProvider mTabProvider;

    private ObservableSupplierImpl<OnClickListener> mShareButtonListenerSupplier =
            new ObservableSupplierImpl<>();
    private CallbackController mCallbackController = new CallbackController();
    ObservableSupplier<AppMenuButtonHelper> mMenuButtonHelperSupplier;
    private BottomControlsMediator mBottomControlsMediator;
    private Runnable mOriginalHomeButtonRunnable;
    private final BraveScrollingBottomViewResourceFrameLayout mScrollingBottomView;
    private HomeButton mHomeButton;
    private BookmarksButton mBookmarksButton;
    private SearchAccelerator mSearchAccelerator;
    private BottomToolbarNewTabButton mNewTabButton;
    private View mBottomContainerTopShadow;

    private final Context mContext = ContextUtils.getApplicationContext();

    BottomToolbarCoordinator(ScrollingBottomViewResourceFrameLayout scrollingBottomView,
            View root, ActivityTabProvider tabProvider,
            OnLongClickListener tabsSwitcherLongClickListner, ThemeColorProvider themeColorProvider,
            Runnable openHomepageAction, Callback<Integer> setUrlBarFocusAction,
            OneshotSupplier<LayoutStateProvider> layoutStateProviderSupplier,
            ObservableSupplier<AppMenuButtonHelper> menuButtonHelperSupplier,
            BottomControlsMediator bottomControlsMediator) {
        layoutStateProviderSupplier.onAvailable(
                mCallbackController.makeCancelable(this::setLayoutStateProvider));

        final OnClickListener homeButtonListener = v -> {
            openHomepageAction.run();
        };

        final OnClickListener searchAcceleratorListener = v -> {
            setUrlBarFocusAction.onResult(OmniboxFocusReason.ACCELERATOR_TAP);
        };

        mBrowsingModeCoordinator = new BrowsingModeBottomToolbarCoordinator(root, tabProvider,
                homeButtonListener, searchAcceleratorListener, mShareButtonListenerSupplier,
                tabsSwitcherLongClickListner);

        mTabSwitcherModeStub = root.findViewById(R.id.bottom_toolbar_tab_switcher_mode_stub);

        mThemeColorProvider = themeColorProvider;
        mTabProvider = tabProvider;

        mMenuButtonHelperSupplier = menuButtonHelperSupplier;
        mBottomControlsMediator = bottomControlsMediator;
        mOriginalHomeButtonRunnable = openHomepageAction;
        mScrollingBottomView = (BraveScrollingBottomViewResourceFrameLayout) scrollingBottomView;

        mBottomContainerTopShadow =
                mScrollingBottomView.findViewById(R.id.bottom_container_top_shadow);
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

        ChromeActivity activity = BraveActivity.getBraveActivity();
        // Do not change bottom bar if StartSurface Single Pane is enabled and HomePage is not
        // customized.
        if (!ReturnToChromeExperimentsUtil.shouldShowStartSurfaceAsTheHomePage(
                    activity != null ? activity : mContext)
                && BottomToolbarVariationManager.shouldBottomToolbarBeVisibleInOverviewMode()) {
            mLayoutStateObserver = new LayoutStateProvider.LayoutStateObserver() {
                @Override
                public void onStartedShowing(@LayoutType int layoutType, boolean showToolbar) {
                    if (layoutType != LayoutType.TAB_SWITCHER) return;

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

                    mBottomContainerTopShadow.setVisibility(View.GONE);
                }

                @Override
                public void onStartedHiding(
                        @LayoutType int layoutType, boolean showToolbar, boolean delayAnimation) {
                    if (layoutType != LayoutType.TAB_SWITCHER) return;

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

                    mBottomContainerTopShadow.setVisibility(View.VISIBLE);
                }
            };
        }

        View root = (View) topToolbarRoot.getParent();
        View bottomToolbarBrowsing = root.findViewById(R.id.bottom_toolbar_browsing);
        View bottomToolbarButtons = root.findViewById(R.id.bottom_toolbar_buttons);

        mHomeButton = bottomToolbarBrowsing.findViewById(R.id.bottom_home_button);
        if (mHomeButton != null) {
            updateHomeButtonState();
            mHomeButton.setOnLongClickListener(this);

            final OnClickListener homeButtonListener = v -> {
                if (HomepageManager.isHomepageEnabled()) {
                    if (BraveActivity.getBraveActivity() != null) {
                        BraveActivity.getBraveActivity().setComesFromNewTab(true);
                    }
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

        if (mScrollingBottomView != null && activity != null) {
            Supplier<CompositorViewHolder> cvh = activity.getCompositorViewHolderSupplier();
            mScrollingBottomView.setSwipeDetector(
                    cvh.get().getLayoutManager().getToolbarSwipeHandler());
        }
    }

    /**
     * @param isVisible Whether the bottom toolbar is visible.
     */
    void setBottomToolbarVisible(boolean isVisible) {
        if (mTabSwitcherModeCoordinator != null) {
            ChromeActivity activity = BraveActivity.getBraveActivity();
            mTabSwitcherModeCoordinator.showToolbarOnTop(!isVisible,
                    TabUiFeatureUtilities.isGridTabSwitcherEnabled(
                            activity != null ? activity : mContext));
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
        if (mLayoutStateProvider != null) {
            mLayoutStateProvider.removeObserver(mLayoutStateObserver);
            mLayoutStateProvider = null;
        }
        mThemeColorProvider.destroy();
    }

    private void setLayoutStateProvider(LayoutStateProvider layoutStateProvider) {
        assert mLayoutStateProvider == null : "the mLayoutStateProvider should set at most once.";

        mLayoutStateProvider = layoutStateProvider;
        mLayoutStateProvider.addObserver(mLayoutStateObserver);
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

    public void updateHomeButtonState() {
        assert (mHomeButton != null);
        if (!HomepageManager.isHomepageEnabled()) {
            mHomeButton.setImageDrawable(
                    ContextCompat.getDrawable(mContext, R.drawable.new_tab_icon));
            mHomeButton.setEnabled(true);
        } else {
            mHomeButton.setImageDrawable(
                    ContextCompat.getDrawable(mContext, R.drawable.btn_toolbar_home));
        }
    }
}
