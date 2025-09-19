/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.components.signin;

import android.accounts.Account;

import org.chromium.build.annotations.NullMarked;
import org.chromium.components.externalauth.ExternalAuthUtils;

/**
 * That class extends upstream's NullAccountManagerDelegate.java to be able run Brave on devices
 * without Google Play Services
 */
@NullMarked
public class BraveNullAccountManagerDelegate extends NullAccountManagerDelegate {
    public BraveNullAccountManagerDelegate() {
        super();
    }

    @Override
    public Account[] getAccountsSynchronous() {
        if (!ExternalAuthUtils.getInstance().canUseGooglePlayServices()) {
            return new Account[] {};
        }

        return super.getAccountsSynchronous();
    }
}
