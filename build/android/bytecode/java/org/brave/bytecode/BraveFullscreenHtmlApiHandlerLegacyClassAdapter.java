/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveFullscreenHtmlApiHandlerLegacyClassAdapter extends BraveClassVisitor {
    static String sFullscreenHtmlApiHandlerLegacy =
            "org/chromium/chrome/browser/fullscreen/FullscreenHtmlApiHandlerLegacy";
    static String sBraveFullscreenHtmlApiHandlerLegacy =
            "org/chromium/chrome/browser/fullscreen/BraveFullscreenHtmlApiHandlerLegacy";

    public BraveFullscreenHtmlApiHandlerLegacyClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(sFullscreenHtmlApiHandlerLegacy, sBraveFullscreenHtmlApiHandlerLegacy);
    }
}
