/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.infobar;

import androidx.annotation.IntDef;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

// Reflects enum items from chromium_src/components/infobars/core/infobar_delegate.h

@IntDef({
    BraveInfoBarIdentifier.INVALID,
    BraveInfoBarIdentifier.BRAVE_CONFIRM_P3A_INFOBAR_DELEGATE,
    BraveInfoBarIdentifier.SYNC_CANNOT_RUN_INFOBAR,
    BraveInfoBarIdentifier.WEB_DISCOVERY_INFOBAR_DELEGATE,
    BraveInfoBarIdentifier.BRAVE_SYNC_ACCOUNT_DELETED_INFOBAR
})
@Retention(RetentionPolicy.SOURCE)
public @interface BraveInfoBarIdentifier {
    int INVALID = -1;
    int BRAVE_CONFIRM_P3A_INFOBAR_DELEGATE = 500;
    // int WAYBACK_MACHINE_INFOBAR_DELEGATE = 502; - deprecated
    //  int SYNC_V2_MIGRATE_INFOBAR_DELEGATE = 503; - deprecated
    //  int ANDROID_SYSTEM_SYNC_DISABLED_INFOBAR = 504; - deprecated
    int SYNC_CANNOT_RUN_INFOBAR = 505;
    int WEB_DISCOVERY_INFOBAR_DELEGATE = 506;
    int BRAVE_SYNC_ACCOUNT_DELETED_INFOBAR = 507;
}
