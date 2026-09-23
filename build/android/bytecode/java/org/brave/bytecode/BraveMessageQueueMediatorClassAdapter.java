/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveMessageQueueMediatorClassAdapter extends BraveClassVisitor {
    static String sChromeMessageQueueMediatorClassName =
            "org/chromium/chrome/browser/messages/ChromeMessageQueueMediator";
    static String sBraveMessageQueueMediatorClassName =
            "org/chromium/chrome/browser/messages/BraveMessageQueueMediator";

    public BraveMessageQueueMediatorClassAdapter(ClassVisitor visitor) {
        super(visitor);
        redirectConstructor(
                sChromeMessageQueueMediatorClassName, sBraveMessageQueueMediatorClassName);
    }
}
