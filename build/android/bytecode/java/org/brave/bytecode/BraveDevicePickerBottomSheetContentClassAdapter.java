/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveDevicePickerBottomSheetContentClassAdapter extends BraveClassVisitor {
    static String sDevicePickerBottomSheetContent =
            "org/chromium/chrome/browser/share/send_tab_to_self/DevicePickerBottomSheetContent";
    static String sBraveDevicePickerBottomSheetContent =
            "org/chromium/chrome/browser/share/send_tab_to_self/BraveDevicePickerBottomSheetContent";

    public BraveDevicePickerBottomSheetContentClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(sDevicePickerBottomSheetContent, sBraveDevicePickerBottomSheetContent);

        makePublicMethod(sDevicePickerBottomSheetContent, "createManageDevicesLink");
        addMethodAnnotation(sBraveDevicePickerBottomSheetContent, "createManageDevicesLink",
                "Ljava/lang/Override;");
    }
}
