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

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.ObservableSupplier;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ActivityTabProvider;
import org.chromium.chrome.browser.ThemeColorProvider;
import org.chromium.chrome.browser.compositor.layouts.EmptyOverviewModeObserver;
import org.chromium.chrome.browser.compositor.layouts.OverviewModeBehavior;
import org.chromium.chrome.browser.partnercustomizations.HomepageManager;
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

    private final Context mContext = ContextUtils.getApplicationContext();

    BraveBottomToolbarCoordinator(ViewStub stub, ActivityTabProvider tabProvider,
            OnClickListener homeButtonListener, OnClickListener searchAcceleratorListener,
            ObservableSupplier<OnClickListener> shareButtonListener,
            OnLongClickListener tabsSwitcherLongClickListner,
            ThemeColorProvider themeColorProvider) {
        super(stub, tabProvider, homeButtonListener, searchAcceleratorListener, shareButtonListener,
                tabsSwitcherLongClickListner, themeColorProvider);
        mBraveTabProvider = tabProvider;
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
            OnClickListener newTabClickListener, OnClickListener closeTabsClickListener,
            AppMenuButtonHelper menuButtonHelper, OverviewModeBehavior overviewModeBehavior,
            TabCountProvider tabCountProvider, IncognitoStateProvider incognitoStateProvider,
            ViewGroup topToolbarRoot) {
        super.initializeWithNative(tabSwitcherListener, newTabClickListener, closeTabsClickListener,
                menuButtonHelper, overviewModeBehavior, tabCountProvider, incognitoStateProvider,
                topToolbarRoot);

        View root = (View) topToolbarRoot.getParent();
        View bottom_toolbar_browsing = root.findViewById(R.id.bottom_toolbar_browsing);
        View bottom_toolbar_buttons = root.findViewById(R.id.bottom_toolbar_buttons);

        mHomeButton = bottom_toolbar_browsing.findViewById(R.id.bottom_home_button);
        if (mHomeButton != null) {
            mHomeButton.setOnLongClickListener(this);

            final OnClickListener homeButtonListener = v -> {
                final boolean isHomepageEnabled = HomepageManager.isHomepageEnabled();
                if (isHomepageEnabled) {
                    TabUtils.openHomepage();
                } else {
                    TabUtils.openNewTab();
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

        if (mOverviewModeBehavior != null && mOverviewModeObserver != null) {
            // We create new observer here so remove previous
            mOverviewModeBehavior.removeOverviewModeObserver(mOverviewModeObserver);
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
    }
}
