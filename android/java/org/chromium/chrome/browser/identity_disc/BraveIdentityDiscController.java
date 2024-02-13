/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.identity_disc;

import android.content.Context;

import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.chrome.browser.lifecycle.ActivityLifecycleDispatcher;
import org.chromium.chrome.browser.profiles.Profile;

/** Brave's implementation for IdentityDiscController. */
public class BraveIdentityDiscController extends IdentityDiscController {
    public BraveIdentityDiscController(
            Context context,
            ActivityLifecycleDispatcher activityLifecycleDispatcher,
            ObservableSupplier<Profile> profileSupplier) {
        super(context, activityLifecycleDispatcher, profileSupplier);
    }

    /*
     * We want to override `IdentityDiscController#calculateButtonData` via asm
     * to avoid enabling identity button on the home page
     * as this button only meant to be used with Google account.
     */
    public void calculateButtonData() {}
}
