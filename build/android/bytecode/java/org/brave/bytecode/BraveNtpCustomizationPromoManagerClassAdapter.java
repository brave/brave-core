/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveNtpCustomizationPromoManagerClassAdapter extends BraveClassVisitor {
    static String sNtpCustomizationPromoManagerClassName =
            "org/chromium/chrome/browser/ntp_customization/theme/NtpCustomizationPromoManager";
    static String sBraveNtpCustomizationPromoManagerClassName =
            "org/chromium/chrome/browser/ntp_customization/theme/BraveNtpCustomizationPromoManager";
    static String sMethodCanShowCustomizationIph = "canShowCustomizationIph";

    public BraveNtpCustomizationPromoManagerClassAdapter(ClassVisitor visitor) {
        super(visitor);
        changeMethodOwner(
                sNtpCustomizationPromoManagerClassName,
                sMethodCanShowCustomizationIph,
                sBraveNtpCustomizationPromoManagerClassName);
    }
}
