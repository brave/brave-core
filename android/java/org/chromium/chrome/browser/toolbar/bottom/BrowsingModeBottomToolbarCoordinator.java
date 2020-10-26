/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.bottom;

import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnLongClickListener;

import org.chromium.base.Callback;
import org.chromium.base.CallbackController;
import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ActivityTabProvider;
import org.chromium.chrome.browser.ActivityTabProvider.HintlessActivityTabObserver;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.compositor.layouts.OverviewModeBehavior;
import org.chromium.chrome.browser.feature_engagement.TrackerFactory;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabImpl;
import org.chromium.chrome.browser.tab.TabUtils;
import org.chromium.chrome.browser.tabmodel.IncognitoStateProvider;
import org.chromium.chrome.browser.tasks.ReturnToChromeExperimentsUtil;
import org.chromium.chrome.browser.toolbar.HomeButton;
import org.chromium.chrome.browser.toolbar.TabCountProvider;
import org.chromium.chrome.browser.toolbar.TabSwitcherButtonCoordinator;
import org.chromium.chrome.browser.toolbar.TabSwitcherButtonView;
import org.chromium.chrome.browser.toolbar.ThemeColorProvider;
import org.chromium.chrome.browser.toolbar.menu_button.MenuButton;
import org.chromium.chrome.browser.ui.appmenu.AppMenuButtonHelper;
import org.chromium.components.feature_engagement.FeatureConstants;
import org.chromium.components.feature_engagement.Tracker;
import org.chromium.ui.modelutil.PropertyModelChangeProcessor;

/**
 * The coordinator for the browsing mode bottom toolbar. This class has two primary components,
 * an Android view that handles user actions and a composited texture that draws when the controls
 * are being scrolled off-screen. The Android version does not draw unless the controls offset is 0.
 */
public class BrowsingModeBottomToolbarCoordinator {
    /** The mediator that handles events from outside the browsing mode bottom toolbar. */
    private final BrowsingModeBottomToolbarMediator mMediator;

    /** The home button that lives in the bottom toolbar. */
    private final HomeButton mHomeButton;

    /** The share button that lives in the bottom toolbar. */
    private final ShareButton mShareButton;

    /** The new tab button that lives in the bottom toolbar. */
    private final BottomToolbarNewTabButton mNewTabButton;

    /** The search accelerator that lives in the bottom toolbar. */
    private final SearchAccelerator mSearchAccelerator;

    /** The tab switcher button component that lives in the bottom toolbar. */
    private final TabSwitcherButtonCoordinator mTabSwitcherButtonCoordinator;

    /** The tab switcher button view that lives in the bottom toolbar. */
    private final TabSwitcherButtonView mTabSwitcherButtonView;

    /** The view group that includes all views shown on browsing mode */
    private final BrowsingModeBottomToolbarLinearLayout mToolbarRoot;

    /** The model for the browsing mode bottom toolbar that holds all of its state. */
    private final BrowsingModeBottomToolbarModel mModel;

    /** The callback to be exectured when the share button on click listener is available. */
    private Callback<OnClickListener> mShareButtonListenerSupplierCallback;

    /** The supplier for the share button on click listener. */
    private ObservableSupplier<OnClickListener> mShareButtonListenerSupplier;

    /** The activity tab provider that used for making the IPH. */
    private final ActivityTabProvider mTabProvider;

    private CallbackController mCallbackController = new CallbackController();
    private final BookmarksButton mBookmarkButton;
    private final MenuButton mMenuButton;

    /**
     * Build the coordinator that manages the browsing mode bottom toolbar.
     * @param root The root {@link View} for locating the views to inflate.
     * @param tabProvider The {@link ActivityTabProvider} used for making the IPH.
     * @param homeButtonListener The {@link OnClickListener} for the home button.
     * @param searchAcceleratorListener The {@link OnClickListener} for the search accelerator.
     * @param shareButtonListener The {@link OnClickListener} for the share button.
     * @param overviewModeBehaviorSupplier Supplier for the overview mode manager.
     */
    BrowsingModeBottomToolbarCoordinator(View root, ActivityTabProvider tabProvider,
            OnClickListener homeButtonListener, OnClickListener searchAcceleratorListener,
            ObservableSupplier<OnClickListener> shareButtonListenerSupplier,
            OnLongClickListener tabSwitcherLongClickListener,
            OneshotSupplier<OverviewModeBehavior> overviewModeBehaviorSupplier) {
        mModel = new BrowsingModeBottomToolbarModel();
        mToolbarRoot = root.findViewById(R.id.bottom_toolbar_browsing);
        mTabProvider = tabProvider;

        PropertyModelChangeProcessor.create(
                mModel, mToolbarRoot, new BrowsingModeBottomToolbarViewBinder());

        mMediator = new BrowsingModeBottomToolbarMediator(mModel);

        mHomeButton = mToolbarRoot.findViewById(R.id.bottom_home_button);
        mHomeButton.setOnClickListener(homeButtonListener);
        mHomeButton.setActivityTabProvider(mTabProvider);

        mNewTabButton = mToolbarRoot.findViewById(R.id.bottom_new_tab_button);

        mShareButton = mToolbarRoot.findViewById(R.id.bottom_share_button);

        mSearchAccelerator = mToolbarRoot.findViewById(R.id.search_accelerator);
        mSearchAccelerator.setOnClickListener(searchAcceleratorListener);

        // TODO(amaralp): Make this adhere to MVC framework.
        mTabSwitcherButtonView = mToolbarRoot.findViewById(R.id.bottom_tab_switcher_button);
        mTabSwitcherButtonCoordinator = new TabSwitcherButtonCoordinator(mTabSwitcherButtonView);

        mTabSwitcherButtonView.setOnLongClickListener(tabSwitcherLongClickListener);
        if (BottomToolbarVariationManager.isNewTabButtonOnBottom()) {
            mNewTabButton.setVisibility(View.VISIBLE);
        }
        if (BottomToolbarVariationManager.isHomeButtonOnBottom()) {
            mHomeButton.setVisibility(View.VISIBLE);
        }

        if (BottomToolbarVariationManager.isTabSwitcherOnBottom()) {
            mTabSwitcherButtonView.setVisibility(View.VISIBLE);
        }
        if (BottomToolbarVariationManager.isShareButtonOnBottom()) {
            mShareButton.setVisibility(View.VISIBLE);
            mShareButtonListenerSupplierCallback = shareButtonListener -> {
                mShareButton.setOnClickListener(shareButtonListener);
            };
            mShareButtonListenerSupplier = shareButtonListenerSupplier;
            mShareButton.setActivityTabProvider(mTabProvider);
            mShareButtonListenerSupplier.addObserver(mShareButtonListenerSupplierCallback);
        }

        overviewModeBehaviorSupplier.onAvailable(
                mCallbackController.makeCancelable((overviewModeBehavior) -> {
                    setOverviewModeBehavior(overviewModeBehavior);
                }));

        mBookmarkButton = mToolbarRoot.findViewById(R.id.bottom_bookmark_button);
        if (BottomToolbarVariationManager.isBookmarkButtonOnBottom()) {
            mBookmarkButton.setVisibility(View.VISIBLE);
            getNewTabButtonParent().setVisibility(View.GONE);
            OnClickListener bookmarkClickHandler = v -> {
                TabImpl tab = (TabImpl) mTabProvider.get();
                BraveActivity activity = BraveActivity.getBraveActivity();
                if (tab == null || activity == null) {
                    assert false;
                    return;
                }
                activity.addOrEditBookmark(tab);
            };
            mBookmarkButton.setOnClickListener(bookmarkClickHandler);
        }

        mMenuButton = mToolbarRoot.findViewById(R.id.menu_button_wrapper);
        if (!BottomToolbarVariationManager.isMenuButtonOnBottom()) {
            mMenuButton.setVisibility(View.GONE);
        }
    }

    /**
     * @param isVisible Whether the bottom toolbar is visible.
     */
    void onVisibilityChanged(boolean isVisible) {}

    /**
     * Initialize the bottom toolbar with the components that had native initialization
     * dependencies.
     * <p>
     * Calling this must occur after the native library have completely loaded.
     * @param tabSwitcherListener An {@link OnClickListener} that is triggered when the
     *                            tab switcher button is clicked.
     * @param menuButtonHelper An {@link AppMenuButtonHelper} that is triggered when the
     *                         menu button is clicked.
     * @param tabCountProvider Updates the tab count number in the tab switcher button.
     * @param themeColorProvider Notifies components when theme color changes.
     * @param incognitoStateProvider Notifies components when incognito state changes.
     */
    void initializeWithNative(OnClickListener newTabListener, OnClickListener tabSwitcherListener,
            AppMenuButtonHelper menuButtonHelper, TabCountProvider tabCountProvider,
            ThemeColorProvider themeColorProvider, IncognitoStateProvider incognitoStateProvider) {
        mMediator.setThemeColorProvider(themeColorProvider);
        if (BottomToolbarVariationManager.isNewTabButtonOnBottom()) {
            mNewTabButton.setOnClickListener(newTabListener);
            mNewTabButton.setThemeColorProvider(themeColorProvider);
            mNewTabButton.setIncognitoStateProvider(incognitoStateProvider);
        }
        if (BottomToolbarVariationManager.isHomeButtonOnBottom()) {
            mHomeButton.setThemeColorProvider(themeColorProvider);
        }

        if (BottomToolbarVariationManager.isShareButtonOnBottom()) {
            mShareButton.setThemeColorProvider(themeColorProvider);
        }

        mSearchAccelerator.setThemeColorProvider(themeColorProvider);
        mSearchAccelerator.setIncognitoStateProvider(incognitoStateProvider);

        if (BottomToolbarVariationManager.isTabSwitcherOnBottom()) {
            mTabSwitcherButtonCoordinator.setTabSwitcherListener(tabSwitcherListener);
            mTabSwitcherButtonCoordinator.setThemeColorProvider(themeColorProvider);
            mTabSwitcherButtonCoordinator.setTabCountProvider(tabCountProvider);
        }

        mBookmarkButton.setThemeColorProvider(themeColorProvider);

        mMenuButton.setAppMenuButtonHelper(menuButtonHelper);
    }

    private void setOverviewModeBehavior(OverviewModeBehavior overviewModeBehavior) {
        assert overviewModeBehavior != null;

        // If StartSurface is HomePage, BrowsingModeBottomToolbar is shown in browsing mode and in
        // overview mode. We need to pass the OverviewModeBehavior to the buttons so they are
        // disabled based on the overview state.
        if (ReturnToChromeExperimentsUtil.shouldShowStartSurfaceAsTheHomePage()) {
            mShareButton.setOverviewModeBehavior(overviewModeBehavior);
            // mTabSwitcherButtonCoordinator.setOverviewModeBehavior(overviewModeBehavior);
            mHomeButton.setOverviewModeBehavior(overviewModeBehavior);
        }
    }

    /**
     * @param enabled Whether to disable click events on the bottom toolbar. Setting true can also
     *                prevent from all click events on toolbar and all children views on toolbar.
     */
    void setTouchEnabled(boolean enabled) {
        mToolbarRoot.setTouchEnabled(enabled);
    }

    /**
     * @param visible Whether to hide the tab switcher bottom toolbar
     */
    void setVisible(boolean visible) {
        mModel.set(BrowsingModeBottomToolbarModel.IS_VISIBLE, visible);
    }

    /**
     * @return The browsing mode bottom toolbar's share button.
     */
    ShareButton getShareButton() {
        return mShareButton;
    }

    /**
     * @return The browsing mode bottom toolbar's tab switcher button.
     */
    TabSwitcherButtonView getTabSwitcherButtonView() {
        return mTabSwitcherButtonView;
    }

    /**
     * @return The browsing mode bottom toolbar's search button.
     */
    SearchAccelerator getSearchAccelerator() {
        return mSearchAccelerator;
    }

    /**
     * @return The browsing mode bottom toolbar's home button.
     */
    HomeButton getHomeButton() {
        return mHomeButton;
    }

    /**
     * Clean up any state when the browsing mode bottom toolbar is destroyed.
     */
    public void destroy() {
        if (mShareButtonListenerSupplier != null) {
            mShareButtonListenerSupplier.removeObserver(mShareButtonListenerSupplierCallback);
        }
        mMediator.destroy();
        mHomeButton.destroy();
        mShareButton.destroy();
        mSearchAccelerator.destroy();
        mTabSwitcherButtonCoordinator.destroy();
        mBookmarkButton.destroy();
    }

    public void updateBookmarkButton(boolean isBookmarked, boolean editingAllowed) {
        if (mBookmarkButton != null) {
            mBookmarkButton.updateBookmarkButton(isBookmarked, editingAllowed);
        }
    }

    View getNewTabButtonParent() {
        return (View)mNewTabButton.getParent();
    }

    BookmarksButton getBookmarkButton() {
        return mBookmarkButton;
    }
}
