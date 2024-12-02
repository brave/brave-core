/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.share.send_tab_to_self;

import android.content.Context;
import android.util.AttributeSet;
import android.view.View;
import android.widget.LinearLayout;

import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.components.signin.base.AccountInfo;

public class BraveManageAccountDevicesLinkView extends LinearLayout {
    public BraveManageAccountDevicesLinkView(Context context, AttributeSet attrs) {
        super(context, attrs);

        setVisibility(View.GONE);
    }

    public static AccountInfo getSharingAccountInfo(Profile profile) {
        return new AccountInfo.Builder("", "").build();
    }
}
