/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveMediaSessionHelperClassAdapter extends BraveClassVisitor {
    static String sMediaSessionHelperClassName =
            "org/chromium/components/browser_ui/media/MediaSessionHelper";

    static String sBraveMediaSessionHelperClassName =
            "org/chromium/components/browser_ui/media/BraveMediaSessionHelper";

    public BraveMediaSessionHelperClassAdapter(ClassVisitor visitor) {
        super(visitor);

        changeSuperName(sMediaSessionHelperClassName, sBraveMediaSessionHelperClassName);

        changeMethodOwner(
                sMediaSessionHelperClassName,
                "showNotification",
                sBraveMediaSessionHelperClassName);
        changeMethodOwner(
                sMediaSessionHelperClassName,
                "createMediaSessionObserver",
                sBraveMediaSessionHelperClassName);
    }
}
