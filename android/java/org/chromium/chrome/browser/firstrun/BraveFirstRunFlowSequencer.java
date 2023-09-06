/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.firstrun;
import android.app.Activity;

import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.components.signin.AccountManagerFacadeProvider;

public abstract class BraveFirstRunFlowSequencer extends FirstRunFlowSequencer {
    public BraveFirstRunFlowSequencer(Activity activity, OneshotSupplier<Profile> profileSupplier) {
        super(activity, profileSupplier,
                new ChildAccountStatusSupplier(AccountManagerFacadeProvider.getInstance(),
                        FirstRunAppRestrictionInfo.takeMaybeInitialized()));
    }

    @Override
    public void start() {
        super.start();
    }
}
