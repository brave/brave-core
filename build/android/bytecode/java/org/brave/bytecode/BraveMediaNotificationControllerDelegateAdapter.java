/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveMediaNotificationControllerDelegateAdapter extends BraveClassVisitor {
    static String sChromeMediaNotificationControllerDelegate =
            "org/chromium/chrome/browser/media/ui/ChromeMediaNotificationControllerDelegate";
    static String sBraveMediaNotificationControllerDelegate =
            "org/chromium/chrome/browser/media/ui/BraveMediaNotificationControllerDelegate";

    public BraveMediaNotificationControllerDelegateAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(
                sChromeMediaNotificationControllerDelegate,
                sBraveMediaNotificationControllerDelegate);
        deleteMethod(sBraveMediaNotificationControllerDelegate, "getContext");
        makePublicMethod(sChromeMediaNotificationControllerDelegate, "getContext");
    }
}
