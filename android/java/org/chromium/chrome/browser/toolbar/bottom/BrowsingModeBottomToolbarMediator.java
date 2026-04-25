/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.bottom;

import android.content.Context;

import androidx.core.content.ContextCompat;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.layouts.LayoutStateProvider;
import org.chromium.chrome.browser.layouts.LayoutStateProvider.LayoutStateObserver;
import org.chromium.chrome.browser.layouts.LayoutType;
import org.chromium.chrome.browser.tabmodel.IncognitoStateProvider;
import org.chromium.chrome.browser.theme.ThemeColorProvider;
import org.chromium.chrome.browser.theme.ThemeColorProvider.ThemeColorObserver;

/**
 * This class is responsible for reacting to events from the outside world, interacting with other
 * coordinators, running most of the business logic associated with the browsing mode bottom
 * toolbar, and updating the model accordingly.
 */
@NullMarked
class BrowsingModeBottomToolbarMediator implements ThemeColorObserver, LayoutStateObserver {
    /** The dismissable parameter name of the IPH. */
    static final String DUET_IPH_TAP_TO_DISMISS_PARAM_NAME = "duet_iph_tap_to_dismiss_enabled";

    /** The model for the browsing mode bottom toolbar that holds all of its state. */
    private final BrowsingModeBottomToolbarModel mModel;

    /** A provider that notifies components when the theme color changes. */
    @Nullable private ThemeColorProvider mThemeColorProvider;

    /** Provider for incognito state. */
    @Nullable private IncognitoStateProvider mIncognitoStateProvider;

    /** Provider for layout state. */
    @Nullable private LayoutStateProvider mLayoutStateProvider;

    /** The application context. */
    private final Context mContext;

    /** The primary color of the bottom toolbar. */
    private int mPrimaryColor = -1;

    /**
     * Build a new mediator that handles events from outside the bottom toolbar.
     *
     * @param model The {@link BrowsingModeBottomToolbarModel} that holds all the state for the
     *     browsing mode bottom toolbar.
     */
    BrowsingModeBottomToolbarMediator(BrowsingModeBottomToolbarModel model, Context context) {
        mModel = model;
        mContext = context;
    }

    void setThemeColorProvider(ThemeColorProvider themeColorProvider) {
        mThemeColorProvider = themeColorProvider;
        mThemeColorProvider.addThemeColorObserver(this);
    }

    void setIncognitoStateProvider(IncognitoStateProvider incognitoStateProvider) {
        mIncognitoStateProvider = incognitoStateProvider;
    }

    void setLayoutStateProvider(LayoutStateProvider layoutStateProvider) {
        mLayoutStateProvider = layoutStateProvider;
        mLayoutStateProvider.addObserver(this);
    }

    /** Clean up anything that needs to be when the bottom toolbar is destroyed. */
    void destroy() {
        if (mThemeColorProvider != null) {
            mThemeColorProvider.removeThemeColorObserver(this);
            mThemeColorProvider = null;
        }
        if (mLayoutStateProvider != null) {
            mLayoutStateProvider.removeObserver(this);
            mLayoutStateProvider = null;
        }
        mIncognitoStateProvider = null;
    }

    @Override
    public void onThemeColorChanged(int primaryColor, boolean shouldAnimate) {
        mPrimaryColor = primaryColor;
        updateColor(primaryColor);
    }

    @Override
    public void onStartedShowing(@LayoutType int layoutType) {
        // Make sure to use the correct color for the bottom toolbar in tab switcher view for
        // private tabs.
        if (mIncognitoStateProvider != null && mIncognitoStateProvider.isIncognitoSelected()) {
            updateColor(
                    layoutType == LayoutType.TAB_SWITCHER
                            ? ContextCompat.getColor(mContext, R.color.default_bg_color_dark)
                            : mPrimaryColor);
        }
    }

    private void updateColor(int primaryColor) {
        mModel.set(BrowsingModeBottomToolbarModel.PRIMARY_COLOR, primaryColor);
    }
}
