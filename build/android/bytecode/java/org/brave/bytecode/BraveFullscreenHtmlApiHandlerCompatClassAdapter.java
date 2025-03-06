/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

 package org.brave.bytecode;

 import org.objectweb.asm.ClassVisitor;

 public class BraveFullscreenHtmlApiHandlerCompatClassAdapter extends BraveClassVisitor {
    static String sFullscreenHtmlApiHandlerCompat =
            "org/chromium/chrome/browser/fullscreen/FullscreenHtmlApiHandlerCompat";
    static String sBraveFullscreenHtmlApiHandlerCompat =
            "org/chromium/chrome/browser/fullscreen/BraveFullscreenHtmlApiHandlerCompat";

    public BraveFullscreenHtmlApiHandlerCompatClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(sFullscreenHtmlApiHandlerCompat, sBraveFullscreenHtmlApiHandlerCompat);
    }
}
