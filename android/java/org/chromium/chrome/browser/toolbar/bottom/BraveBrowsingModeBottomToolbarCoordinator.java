/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.bottom;

import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnLongClickListener;

import org.chromium.base.Callback;
import org.chromium.base.ObservableSupplier;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ActivityTabProvider;
import org.chromium.chrome.browser.compositor.layouts.OverviewModeBehavior;
import org.chromium.chrome.browser.ThemeColorProvider;
import org.chromium.chrome.browser.toolbar.IncognitoStateProvider;
import org.chromium.chrome.browser.toolbar.MenuButton;
import org.chromium.chrome.browser.toolbar.TabCountProvider;
import org.chromium.chrome.browser.ui.appmenu.AppMenuButtonHelper;

/**
 * Brave's extention to BrowsingModeBottomToolbarCoordinator.
 */
public class BraveBrowsingModeBottomToolbarCoordinator
        extends BrowsingModeBottomToolbarCoordinator {
    private Callback<OnClickListener> mBookmarkButtonListenerSupplierCallback;
    private ObservableSupplier<OnClickListener> mBookmarkButtonListenerSupplier;
    private final BrowsingModeBottomToolbarLinearLayout mBraveToolbarRoot;
    private final ActivityTabProvider mBraveTabProvider;
    private final BookmarksButton mBookmarkButton;
    private final MenuButton mMenuButton;
    private final BottomToolbarNewTabButton mBraveNewTabButton;

    BraveBrowsingModeBottomToolbarCoordinator(View root, ActivityTabProvider tabProvider,
            OnClickListener homeButtonListener, OnClickListener searchAcceleratorListener,
            ObservableSupplier<OnClickListener> shareButtonListenerSupplier,
            OnLongClickListener tabSwitcherLongClickListener) {
        super(root, tabProvider, homeButtonListener, searchAcceleratorListener,
                shareButtonListenerSupplier, tabSwitcherLongClickListener);
        mBraveTabProvider = tabProvider;
        mBraveToolbarRoot = root.findViewById(R.id.bottom_toolbar_browsing);
        mBraveNewTabButton = mBraveToolbarRoot.findViewById(R.id.bottom_new_tab_button);
        mBookmarkButton = mBraveToolbarRoot.findViewById(R.id.bottom_bookmark_button);
        if (BottomToolbarVariationManager.isBookmarkButtonOnBottom()) {
            mBookmarkButton.setVisibility(View.VISIBLE);
            getNewTabButtonParent().setVisibility(View.GONE);
        }
        if (BottomToolbarVariationManager.isBookmarkButtonOnBottom()) {
            mBookmarkButton.setVisibility(View.VISIBLE);
            mBookmarkButtonListenerSupplierCallback = bookmarkButtonListener -> {
                mBookmarkButton.setOnClickListener(bookmarkButtonListener);
            };
            // To avoid extensive patching we use shareButtonListenerSupplier for both
            // share and bookmark buttons. Thus we can't use them both simultaneously.
            mBookmarkButtonListenerSupplier = shareButtonListenerSupplier;
            mBookmarkButton.setActivityTabProvider(mBraveTabProvider);
            mBookmarkButtonListenerSupplier.addObserver(mBookmarkButtonListenerSupplierCallback);
        }
        mMenuButton = mBraveToolbarRoot.findViewById(R.id.menu_button_wrapper);
        if (!BottomToolbarVariationManager.isMenuButtonOnBottom()) {
            mMenuButton.setVisibility(View.GONE);
        }
    }

    @Override
    public void initializeWithNative(OnClickListener newTabListener, OnClickListener tabSwitcherListener,
            AppMenuButtonHelper menuButtonHelper, TabCountProvider tabCountProvider,
            ThemeColorProvider themeColorProvider, IncognitoStateProvider incognitoStateProvider,
            OverviewModeBehavior overviewModeBehavior) {
        super.initializeWithNative(newTabListener, tabSwitcherListener,
                menuButtonHelper, tabCountProvider, themeColorProvider,
                incognitoStateProvider, overviewModeBehavior);
        mBookmarkButton.setThemeColorProvider(themeColorProvider);

        mMenuButton.setAppMenuButtonHelper(menuButtonHelper);
        mMenuButton.setThemeColorProvider(themeColorProvider);
    }

    @Override
    public void updateBookmarkButton(boolean isBookmarked, boolean editingAllowed) {
        if (mBookmarkButton != null) {
            mBookmarkButton.updateBookmarkButton(isBookmarked, editingAllowed);
        }
    }

    @Override
    public void destroy() {
        super.destroy();
        if (mBookmarkButtonListenerSupplier != null) {
            mBookmarkButtonListenerSupplier.removeObserver(mBookmarkButtonListenerSupplierCallback);
        }
        mBookmarkButton.destroy();
        mMenuButton.destroy();
    }

    View getNewTabButtonParent() {
        return (View)mBraveNewTabButton.getParent();
    }

    BookmarksButton getBookmarkButton() {
        return mBookmarkButton;
    }
}
