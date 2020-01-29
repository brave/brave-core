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
import org.chromium.chrome.browser.compositor.layouts.OverviewModeBehavior;
import org.chromium.chrome.browser.toolbar.bottom.BottomToolbarNewTabButton;
import org.chromium.chrome.browser.toolbar.bottom.SearchAccelerator;
import org.chromium.chrome.browser.toolbar.bottom.ShareButton;
import org.chromium.chrome.browser.toolbar.HomeButton;
import org.chromium.chrome.browser.toolbar.IncognitoStateProvider;
import org.chromium.chrome.browser.toolbar.TabCountProvider;
import org.chromium.chrome.browser.ui.appmenu.AppMenuButtonHelper;
import org.chromium.ui.widget.Toast;

public class BraveBottomToolbarCoordinator
        extends BottomToolbarCoordinator implements View.OnLongClickListener {
    private HomeButton mHomeButton;
    private ShareButton mBookmarksButton;
    private SearchAccelerator mSearchAccelerator;
    private BottomToolbarNewTabButton mNewTabButton;;

    private final Context mContext = ContextUtils.getApplicationContext();

    BraveBottomToolbarCoordinator(ViewStub stub, ActivityTabProvider tabProvider,
            OnClickListener homeButtonListener, OnClickListener searchAcceleratorListener,
            ObservableSupplier<OnClickListener> shareButtonListener,
            OnLongClickListener tabsSwitcherLongClickListner,
            ThemeColorProvider themeColorProvider) {
        super(stub, tabProvider, homeButtonListener, searchAcceleratorListener, shareButtonListener,
                tabsSwitcherLongClickListner, themeColorProvider);
    }

    @Override
    public boolean onLongClick(View v) {
        String description = "";
        Resources resources = mContext.getResources();

        if (v == mHomeButton) {
            description = resources.getString(R.string.accessibility_toolbar_btn_home);
        } else if (v == mBookmarksButton) {
            description = resources.getString(R.string.accessibility_toolbar_btn_bookmark);
        } else if (v == mSearchAccelerator) {
            description =
                    resources.getString(R.string.accessibility_toolbar_btn_search_accelerator);
        } else if (v == mNewTabButton) {
            description = resources.getString(R.string.accessibility_new_tab_page);
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

        mHomeButton = bottom_toolbar_browsing.findViewById(R.id.home_button);
        if (mHomeButton != null) {
            mHomeButton.setOnLongClickListener(this);
        }

        mBookmarksButton = bottom_toolbar_browsing.findViewById(R.id.bottom_share_button);
        if (mBookmarksButton != null) {
            mBookmarksButton.setOnLongClickListener(this);
        }

        mSearchAccelerator = bottom_toolbar_browsing.findViewById(R.id.search_accelerator);
        if (mSearchAccelerator != null) {
            mSearchAccelerator.setOnLongClickListener(this);
        }

        mNewTabButton = bottom_toolbar_buttons.findViewById(R.id.new_tab_button);
        if (mNewTabButton != null) {
            mNewTabButton.setOnLongClickListener(this);
        }
    }
}
