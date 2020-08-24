/* Copyright (c) 2019 The Brave Authors. All rights reserved.
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
import org.chromium.base.ContextUtils;
import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.Supplier;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ActivityTabProvider;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.ThemeColorProvider;
import org.chromium.chrome.browser.compositor.layouts.EmptyOverviewModeObserver;
import org.chromium.chrome.browser.compositor.layouts.OverviewModeBehavior;
import org.chromium.chrome.browser.homepage.HomepageManager;
import org.chromium.chrome.browser.share.ShareDelegate;
import org.chromium.chrome.browser.toolbar.HomeButton;
import org.chromium.chrome.browser.toolbar.IncognitoStateProvider;
import org.chromium.chrome.browser.toolbar.TabCountProvider;
import org.chromium.chrome.browser.toolbar.bottom.BottomToolbarNewTabButton;
import org.chromium.chrome.browser.toolbar.bottom.SearchAccelerator;
import org.chromium.chrome.browser.toolbar.bottom.ShareButton;
import org.chromium.chrome.browser.ui.appmenu.AppMenuButtonHelper;
import org.chromium.chrome.browser.util.TabUtils;
import org.chromium.ui.widget.Toast;

public class BraveBottomToolbarCoordinator
        extends BottomToolbarCoordinator implements View.OnLongClickListener {
    private HomeButton mHomeButton;
    private ShareButton mBookmarksButton;
    private SearchAccelerator mSearchAccelerator;
    private BottomToolbarNewTabButton mNewTabButton;
    private ActivityTabProvider mBraveTabProvider;
    private Runnable mOriginalHomeButtonRunnable;
    private final ScrollingBottomViewResourceFrameLayout rootView;

    private final Context mContext = ContextUtils.getApplicationContext();

    BraveBottomToolbarCoordinator(ScrollingBottomViewResourceFrameLayout root, ViewStub stub,
            ActivityTabProvider tabProvider, OnLongClickListener tabsSwitcherLongClickListner,
            ThemeColorProvider themeColorProvider,
            ObservableSupplier<ShareDelegate> shareDelegateSupplier,
            Supplier<Boolean> showStartSurfaceCallable, Runnable openHomepageAction,
            Callback<Integer> setUrlBarFocusAction,
            ObservableSupplier<OverviewModeBehavior> overviewModeBehaviorSupplier,
            ObservableSupplier<AppMenuButtonHelper> menuButtonHelperSupplier) {
        super(stub, tabProvider, tabsSwitcherLongClickListner, themeColorProvider,
                shareDelegateSupplier, showStartSurfaceCallable, openHomepageAction,
                setUrlBarFocusAction, overviewModeBehaviorSupplier, menuButtonHelperSupplier);
        mBraveTabProvider = tabProvider;
        mOriginalHomeButtonRunnable = openHomepageAction;
        rootView = root;
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

    @Override
    void initializeWithNative(OnClickListener tabSwitcherListener,
            OnClickListener newTabClickListener, TabCountProvider tabCountProvider,
            IncognitoStateProvider incognitoStateProvider, ViewGroup topToolbarRoot,
            Runnable closeAllTabsAction) {
        super.initializeWithNative(tabSwitcherListener, newTabClickListener, tabCountProvider,
                incognitoStateProvider, topToolbarRoot, closeAllTabsAction);

        View root = (View) topToolbarRoot.getParent();
        View bottom_toolbar_browsing = root.findViewById(R.id.bottom_toolbar_browsing);
        View bottom_toolbar_buttons = root.findViewById(R.id.bottom_toolbar_buttons);

        mHomeButton = bottom_toolbar_browsing.findViewById(R.id.bottom_home_button);
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

        mBookmarksButton = bottom_toolbar_browsing.findViewById(R.id.bottom_bookmark_button);
        if (mBookmarksButton != null) {
            mBookmarksButton.setOnLongClickListener(this);
        }

        mSearchAccelerator = bottom_toolbar_browsing.findViewById(R.id.search_accelerator);
        if (mSearchAccelerator != null) {
            mSearchAccelerator.setOnLongClickListener(this);
        }

        mNewTabButton = bottom_toolbar_buttons.findViewById(R.id.bottom_new_tab_button);
        if (mNewTabButton != null) {
            mNewTabButton.setOnLongClickListener(this);
        }

        if (mOverviewModeObserver != null) {
            if (mOverviewModeBehavior != null) {
                // We create new observer here so remove previous
                mOverviewModeBehavior.removeOverviewModeObserver(mOverviewModeObserver);
            }
            mOverviewModeObserver = new EmptyOverviewModeObserver() {
                @Override
                public void onOverviewModeStartedShowing(boolean showToolbar) {
                    BraveBrowsingModeBottomToolbarCoordinator browsingModeCoordinator =
                            (BraveBrowsingModeBottomToolbarCoordinator)mBrowsingModeCoordinator;
                    browsingModeCoordinator.getSearchAccelerator().setVisibility(View.GONE);
                    if (BottomToolbarVariationManager.isShareButtonOnBottom()) {
                        browsingModeCoordinator.getShareButton().setVisibility(View.GONE);
                    }
                    if (BottomToolbarVariationManager.isHomeButtonOnBottom()) {
                        browsingModeCoordinator.getHomeButton().setVisibility(View.INVISIBLE);
                    }
                    if (BraveBottomToolbarVariationManager.isBraveVariation()) {
                        browsingModeCoordinator.getBookmarkButton().setVisibility(View.INVISIBLE);
                    }
                    if (BottomToolbarVariationManager.isTabSwitcherOnBottom()) {
                        browsingModeCoordinator.getTabSwitcherButtonView().setVisibility(View.INVISIBLE);
                    }
                    if (BottomToolbarVariationManager.isNewTabButtonOnBottom()) {
                        browsingModeCoordinator.getNewTabButtonParent().setVisibility(View.VISIBLE);
                    }
                }

                @Override
                public void onOverviewModeStartedHiding(
                        boolean showToolbar, boolean delayAnimation) {
                    BraveBrowsingModeBottomToolbarCoordinator browsingModeCoordinator =
                            (BraveBrowsingModeBottomToolbarCoordinator)mBrowsingModeCoordinator;
                    browsingModeCoordinator.getSearchAccelerator().setVisibility(View.VISIBLE);
                    if (BottomToolbarVariationManager.isShareButtonOnBottom()) {
                        browsingModeCoordinator.getShareButton().setVisibility(View.VISIBLE);
                        browsingModeCoordinator.getShareButton().updateButtonEnabledState(
                                mBraveTabProvider.get());
                    }
                    if (BottomToolbarVariationManager.isHomeButtonOnBottom()) {
                        browsingModeCoordinator.getHomeButton().setVisibility(View.VISIBLE);
                        browsingModeCoordinator.getHomeButton().updateButtonEnabledState(
                                mBraveTabProvider.get());
                    }
                    if (BraveBottomToolbarVariationManager.isBraveVariation()) {
                        browsingModeCoordinator.getBookmarkButton().setVisibility(View.VISIBLE);
                    }
                    if (BottomToolbarVariationManager.isTabSwitcherOnBottom()) {
                        browsingModeCoordinator.getTabSwitcherButtonView().setVisibility(View.VISIBLE);
                    }
                    if (BottomToolbarVariationManager.isNewTabButtonOnBottom()) {
                        browsingModeCoordinator.getNewTabButtonParent().setVisibility(View.GONE);
                    }
                }
            };
            if (mOverviewModeBehavior != null) {
                mOverviewModeBehavior.addOverviewModeObserver(mOverviewModeObserver);
            }
        }
        ChromeActivity activity = TabUtils.getChromeActivity();
        if (rootView != null && activity != null) {
            rootView.setSwipeDetector(
                        activity.getCompositorViewHolder().getLayoutManager().getToolbarSwipeHandler());
        }
    }
}
