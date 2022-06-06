/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.share.send_tab_to_self;

import android.content.Context;
import android.util.AttributeSet;

public class BraveManageAccountDevicesLinkView extends ManageAccountDevicesLinkView {
    public BraveManageAccountDevicesLinkView(Context context) {
        super(context, null);
    }

    public void inflateIfVisible() {
        // Do nothing as this feature requires Google account.
    }
}
