/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveNewBackgroundTabAnimationDataClassAdapter extends BraveClassVisitor {
    static String sNewBackgroundTabAnimationData =
            "org/chromium/chrome/browser/compositor/layouts/phone/NewBackgroundTabAnimationData";

    static String sBraveNewBackgroundTabAnimationData =
            "org/chromium/chrome/browser/compositor/layouts/phone/BraveNewBackgroundTabAnimationData"; // presubmit: ignore-long-line

    public BraveNewBackgroundTabAnimationDataClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(sNewBackgroundTabAnimationData, sBraveNewBackgroundTabAnimationData);
    }
}
