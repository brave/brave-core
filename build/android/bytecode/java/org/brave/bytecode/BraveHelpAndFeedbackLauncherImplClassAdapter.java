/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveHelpAndFeedbackLauncherImplClassAdapter extends BraveClassVisitor {
    static String sHelpAndFeedbackLauncherImplClassName =
            "org/chromium/chrome/browser/feedback/HelpAndFeedbackLauncherImpl";

    static String sBraveHelpAndFeedbackLauncherImplClassName =
            "org/chromium/chrome/browser/feedback/BraveHelpAndFeedbackLauncherImpl";

    public BraveHelpAndFeedbackLauncherImplClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(
                sHelpAndFeedbackLauncherImplClassName, sBraveHelpAndFeedbackLauncherImplClassName);
    }
}
