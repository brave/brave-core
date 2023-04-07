/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.incognito.reauth;

import android.content.Context;
import android.view.View;
import android.view.ViewGroup.MarginLayoutParams;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.widget.ImageViewCompat;

import org.chromium.chrome.browser.incognito.R;
import org.chromium.chrome.browser.incognito.reauth.IncognitoReauthManager.IncognitoReauthCallback;
import org.chromium.components.browser_ui.widget.listmenu.ListMenuButtonDelegate;
import org.chromium.ui.base.ViewUtils;
import org.chromium.ui.widget.ButtonCompat;
import org.chromium.ui.widget.ChromeImageView;

abstract class BravePrivateTabReauthCoordinatorBase extends IncognitoReauthCoordinatorBase {
    private static final float REAUTH_UNLOCK_BUTTON_BOTTOM_MARGIN_DP = 24F;

    // To delete in bytecode, variable from the parent class will be used instead.
    private View mIncognitoReauthView;

    public BravePrivateTabReauthCoordinatorBase(@NonNull Context context,
            @NonNull IncognitoReauthManager incognitoReauthManager,
            @NonNull IncognitoReauthCallback incognitoReauthCallback,
            @NonNull Runnable seeOtherTabsRunnable) {
        super(context, incognitoReauthManager, incognitoReauthCallback, seeOtherTabsRunnable);
    }

    @Override
    protected void prepareToshow(
            @Nullable ListMenuButtonDelegate menuButtonDelegate, boolean fullscreen) {
        super.prepareToshow(menuButtonDelegate, fullscreen);

        if (mIncognitoReauthView == null) return;

        ChromeImageView logo = mIncognitoReauthView.findViewById(R.id.incognito_reauth_icon);
        assert logo != null;
        if (logo != null) ImageViewCompat.setImageTintList(logo, null);

        ButtonCompat button =
                mIncognitoReauthView.findViewById(R.id.incognito_reauth_unlock_incognito_button);
        assert button != null;
        if (button != null) {
            MarginLayoutParams params = (MarginLayoutParams) button.getLayoutParams();
            params.bottomMargin = ViewUtils.dpToPx(mContext, REAUTH_UNLOCK_BUTTON_BOTTOM_MARGIN_DP);
            button.setLayoutParams(params);
        }
    }
}
