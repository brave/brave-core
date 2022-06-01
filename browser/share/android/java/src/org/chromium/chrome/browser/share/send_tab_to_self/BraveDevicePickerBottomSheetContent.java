/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.share.send_tab_to_self;

import android.content.Context;
import android.widget.ListView;

import org.chromium.components.browser_ui.bottomsheet.BottomSheetController;

import java.util.List;

public class BraveDevicePickerBottomSheetContent extends DevicePickerBottomSheetContent {
    public BraveDevicePickerBottomSheetContent(Context context, String url, String title,
            BottomSheetController controller, List<TargetDeviceInfo> targetDevices) {
        super(context, url, title, controller, targetDevices);
    }

    public void createManageDevicesLink(ListView deviceListView) {
        // Do nothing as this feature requires Google account.
    }
}
