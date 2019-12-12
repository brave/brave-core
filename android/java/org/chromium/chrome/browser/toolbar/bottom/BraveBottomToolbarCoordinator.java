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
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ActivityTabProvider;
import org.chromium.chrome.browser.ThemeColorProvider;
import org.chromium.chrome.browser.appmenu.AppMenuButtonHelper;
import org.chromium.chrome.browser.compositor.layouts.OverviewModeBehavior;
import org.chromium.chrome.browser.toolbar.IncognitoStateProvider;
import org.chromium.chrome.browser.toolbar.TabCountProvider;
import org.chromium.ui.widget.Toast;

public class BraveBottomToolbarCoordinator
        extends BottomToolbarCoordinator implements View.OnLongClickListener {
    private View mHomeButtonWrapper;
    private View mBookmarksButtonWrapper;
    private View mSearchAcceleratorWrapper;
    private View mNewTabButtonWrapper;

    private final Context mContext = ContextUtils.getApplicationContext();

    BraveBottomToolbarCoordinator(ViewStub stub, ActivityTabProvider tabProvider,
            OnClickListener homeButtonListener, OnClickListener searchAcceleratorListener,
            OnClickListener shareButtonListener, OnLongClickListener tabsSwitcherLongClickListner,
            ThemeColorProvider themeColorProvider) {
        super(stub, tabProvider, homeButtonListener, searchAcceleratorListener, shareButtonListener,
                tabsSwitcherLongClickListner, themeColorProvider);
    }

    @Override
    public boolean onLongClick(View v) {
        String description = "";
        Resources resources = mContext.getResources();

        if (v == mHomeButtonWrapper) {
            description = resources.getString(R.string.accessibility_toolbar_btn_home);
        } else if (v == mBookmarksButtonWrapper) {
            description = resources.getString(R.string.accessibility_toolbar_btn_bookmark);
        } else if (v == mSearchAcceleratorWrapper) {
            description =
                    resources.getString(R.string.accessibility_toolbar_btn_search_accelerator);
        } else if (v == mNewTabButtonWrapper) {
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

        mHomeButtonWrapper = bottom_toolbar_browsing.findViewById(R.id.home_button_wrapper);
        if (mHomeButtonWrapper != null) {
            mHomeButtonWrapper.setOnLongClickListener(this);
        }

        mBookmarksButtonWrapper = bottom_toolbar_browsing.findViewById(R.id.share_button_wrapper);
        if (mBookmarksButtonWrapper != null) {
            mBookmarksButtonWrapper.setOnLongClickListener(this);
        }

        mSearchAcceleratorWrapper =
                bottom_toolbar_browsing.findViewById(R.id.search_accelerator_wrapper);
        if (mSearchAcceleratorWrapper != null) {
            mSearchAcceleratorWrapper.setOnLongClickListener(this);
        }

        mNewTabButtonWrapper = bottom_toolbar_buttons.findViewById(R.id.new_tab_button_wrapper);
        if (mNewTabButtonWrapper != null) {
            mNewTabButtonWrapper.setOnLongClickListener(this);
        }
    }
}
