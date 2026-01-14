/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveTabbedAdaptiveToolbarBehaviorClassAdapter extends BraveClassVisitor {
    private static final String sTabbedAdaptiveToolbarBehavior =
            "org/chromium/chrome/browser/tabbed_mode/TabbedAdaptiveToolbarBehavior";
    private static final String sBraveTabbedAdaptiveToolbarBehavior =
            "org/chromium/chrome/browser/tabbed_mode/BraveTabbedAdaptiveToolbarBehavior";

    public BraveTabbedAdaptiveToolbarBehaviorClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(sTabbedAdaptiveToolbarBehavior, sBraveTabbedAdaptiveToolbarBehavior);
    }
}
