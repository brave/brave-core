/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.sync.settings;

import android.content.Context;
import android.view.View;

import androidx.fragment.app.FragmentManager;

import org.chromium.base.Callback;
import org.chromium.chrome.browser.password_manager.settings.PasswordAccessReauthenticationHelper;
import org.chromium.chrome.browser.password_manager.settings.ReauthenticationManager;
import org.chromium.chrome.browser.password_manager.settings.ReauthenticationManager.ReauthScope;

/**
 * Class to replace description at Chromium's PasswordAccessReauthenticationHelper.reauthenticate
 */
public class BravePasswordAccessReauthenticationHelper
        extends PasswordAccessReauthenticationHelper {
    // Both fields below belong to PasswordAccessReauthenticationHelper and required to make
    // protected with BravePasswordAccessReauthenticationHelperClassAdapter
    private Callback<Boolean> mCallback;
    private FragmentManager mFragmentManager;

    public BravePasswordAccessReauthenticationHelper(
            Context context, FragmentManager fragmentManager) {
        super(context, fragmentManager);
    }

    public void reauthenticateWithDescription(int descriptionId, Callback<Boolean> callback) {
        assert canReauthenticate();
        assert mCallback == null;

        assert mFragmentManager != null;

        // Invoke the handler immediately if an authentication is still valid.
        if (ReauthenticationManager.authenticationStillValid(ReauthScope.ONE_AT_A_TIME)) {
            callback.onResult(true);
            return;
        }

        mCallback = callback;
        ReauthenticationManager.displayReauthenticationFragment(
                descriptionId, View.NO_ID, mFragmentManager, ReauthScope.ONE_AT_A_TIME);
    }
}
