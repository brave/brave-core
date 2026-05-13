/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.incognito.reauth;

import android.app.Activity;
import android.view.View;
import android.view.ViewGroup;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.incognito.R;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.ui.modaldialog.DialogDismissalCause;
import org.chromium.ui.widget.ButtonCompat;

/**
 * Coordinator for the browser-level lock screen. Mirrors the structure of
 * TabSwitcherIncognitoReauthCoordinator but renders as a decor-view overlay instead of inside the
 * tab switcher.
 *
 * <p>Owns its own {@link IncognitoReauthManager} so the underlying {@link
 * android.hardware.biometrics.BiometricPrompt} is always bound to the activity that is actually
 * visible — not the tabbed activity that may be stopped in the background.
 */
@NullMarked
public class BraveBrowserLockCoordinator extends IncognitoReauthCoordinatorBase {
    private final Activity mActivity;
    private @Nullable View mLockView;

    public BraveBrowserLockCoordinator(
            Activity activity,
            Profile profile,
            IncognitoReauthManager.IncognitoReauthCallback incognitoReauthCallback) {
        this(activity, new IncognitoReauthManager(activity, profile), incognitoReauthCallback);
    }

    private BraveBrowserLockCoordinator(
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
        // fullscreen=false avoids the menu-delegate assertion and sets IS_SEE_OTHER_TABS_VISIBLE to
        // false, hiding both the "See other tabs" link and the 3-dot menu from the shared view.
        prepareToShow(/* menuButtonDelegate= */ null, /* fullscreen= */ false);

        mLockView = getIncognitoReauthView();
        assert mLockView != null;

        ButtonCompat unlockButton =
                mLockView.findViewById(R.id.incognito_reauth_unlock_incognito_button);
        unlockButton.setText(org.chromium.chrome.R.string.brave_browser_lock_unlock_button);

        ViewGroup rootView = (ViewGroup) mActivity.getWindow().getDecorView();
        rootView.addView(mLockView);
    }

    @Override
    public void hide(@DialogDismissalCause int dismissalCause) {
        if (mLockView != null) {
            ViewGroup rootView = (ViewGroup) mActivity.getWindow().getDecorView();
            rootView.removeView(mLockView);
            mLockView = null;
            destroy();
        }
    }
}
