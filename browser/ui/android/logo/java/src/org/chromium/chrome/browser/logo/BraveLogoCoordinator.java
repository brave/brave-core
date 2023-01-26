/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.logo;

import android.content.Context;
import android.view.View;

import org.chromium.base.Callback;
import org.chromium.chrome.browser.logo.LogoBridge.Logo;
import org.chromium.content_public.browser.LoadUrlParams;
import org.chromium.ui.modelutil.PropertyModel;

public class BraveLogoCoordinator extends LogoCoordinator {
    // To delete in bytecode, members from parent class will be used instead.
    private PropertyModel mLogoModel;

    // Own members.
    private final LogoView mLogoView;

    public BraveLogoCoordinator(Context context, Callback<LoadUrlParams> logoClickedCallback,
            LogoView logoView, boolean shouldFetchDoodle, Callback<Logo> onLogoAvailableCallback,
            Runnable onCachedLogoRevalidatedRunnable, boolean isParentSurfaceShown,
            LogoCoordinator.VisibilityObserver visibilityObserver) {
        super(context, logoClickedCallback, logoView, shouldFetchDoodle, onLogoAvailableCallback,
                onCachedLogoRevalidatedRunnable, isParentSurfaceShown, visibilityObserver);

        mLogoView = logoView;
    }

    @Override
    public void updateVisibilityAndMaybeCleanUp(
            boolean isParentSurfaceShown, boolean shouldDestroyBridge, boolean animationEnabled) {
        // We don't want any logo to be shown regardless of the search engine chosen.
        mLogoModel.set(LogoProperties.VISIBILITY, false);

        mLogoView.setVisibility(View.GONE);

        super.updateVisibilityAndMaybeCleanUp(
                isParentSurfaceShown, shouldDestroyBridge, animationEnabled);
    }
}
