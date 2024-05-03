/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.components.signin;

import android.accounts.Account;

/**
 * That class extends upstream's SystemAccountManagerDelegate.java to be able run Brave on devices
 * without Google Play Services
 */
public class BraveSystemAccountManagerDelegate extends SystemAccountManagerDelegate {
    public BraveSystemAccountManagerDelegate() {
        super();
    }

    @Override
    public Account[] getAccountsSynchronous() throws AccountManagerDelegateException {
        if (!isGooglePlayServicesAvailable()) {
            return new Account[] {};
        }

        return super.getAccountsSynchronous();
    }
}
