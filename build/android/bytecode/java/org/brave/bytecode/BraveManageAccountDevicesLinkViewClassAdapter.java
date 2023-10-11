/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveManageAccountDevicesLinkViewClassAdapter extends BraveClassVisitor {
    static String sManageAccountDevicesLinkView =
            "org/chromium/chrome/browser/share/send_tab_to_self/ManageAccountDevicesLinkView";
    static String sBraveManageAccountDevicesLinkView =
            "org/chromium/chrome/browser/share/send_tab_to_self/BraveManageAccountDevicesLinkView";

    public BraveManageAccountDevicesLinkViewClassAdapter(ClassVisitor visitor) {
        super(visitor);

        changeSuperName(sManageAccountDevicesLinkView, sBraveManageAccountDevicesLinkView);
    }
}
