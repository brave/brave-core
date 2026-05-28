/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.incognito.reauth;

import android.app.Activity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.incognito.R;
import org.chromium.ui.modaldialog.DialogDismissalCause;
import org.chromium.ui.widget.ButtonCompat;

/**
 * Coordinator for the browser-level lock screen. Renders as a decor-view overlay so it works for
 * any foreground activity (tabbed browser, Settings, etc.).
 *
 * <p>The {@link IncognitoReauthManager} and auth callback are owned by {@link
 * org.chromium.chrome.browser.privacy.BraveBrowserLockManager}; they are passed here only so the
 * base class can wire the unlock button — this class defines neither.
 */
@NullMarked
public class BraveBrowserLockCoordinator extends IncognitoReauthCoordinatorBase {
    private final Activity mActivity;
    private @Nullable View mLockView;

    public BraveBrowserLockCoordinator(
            Activity activity,
            IncognitoReauthManager incognitoReauthManager,
            IncognitoReauthManager.IncognitoReauthCallback incognitoReauthCallback) {
        super(
                activity,
                incognitoReauthManager,
                incognitoReauthCallback,
                /* seeOtherTabsRunnable= */ () -> {});
        mActivity = activity;
    }

    @Override
    public void show() {
        prepareToShow(/* menuButtonDelegate= */ null, /* fullscreen= */ false);

        mLockView = getIncognitoReauthView();
        assert mLockView != null;

        // Override the two text values that differ from the private-tabs defaults.
        TextView statusText =
                mLockView.findViewById(org.chromium.chrome.R.id.incognito_lock_status_text);
        if (statusText != null) {
            statusText.setText(org.chromium.chrome.R.string.brave_browser_locked);
        }
        ButtonCompat unlockButton =
                mLockView.findViewById(R.id.incognito_reauth_unlock_incognito_button);
        unlockButton.setText(org.chromium.chrome.R.string.brave_browser_lock_unlock_button);

        ((ViewGroup) mActivity.getWindow().getDecorView()).addView(mLockView);
    }

    @Override
    public void hide(@DialogDismissalCause int dismissalCause) {
        if (mLockView != null) {
            ((ViewGroup) mActivity.getWindow().getDecorView()).removeView(mLockView);
            mLockView = null;
            destroy();
        }
    }
}
