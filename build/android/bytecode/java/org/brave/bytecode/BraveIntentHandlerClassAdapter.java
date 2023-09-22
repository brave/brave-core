/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveIntentHandlerClassAdapter extends BraveClassVisitor {
    static String sIntentHandlerClassName = "org/chromium/chrome/browser/IntentHandler";
    static String sBraveIntentHandlerClassName = "org/chromium/chrome/browser/BraveIntentHandler";

    public BraveIntentHandlerClassAdapter(ClassVisitor visitor) {
        super(visitor);

        makePublicMethod(sIntentHandlerClassName, "getUrlForCustomTab");
        changeMethodOwner(
                sBraveIntentHandlerClassName, "getUrlForCustomTab", sIntentHandlerClassName);

        makePublicMethod(sIntentHandlerClassName, "getUrlForWebapp");
        changeMethodOwner(sBraveIntentHandlerClassName, "getUrlForWebapp", sIntentHandlerClassName);

        makePublicMethod(sIntentHandlerClassName, "isJavascriptSchemeOrInvalidUrl");
        changeMethodOwner(sBraveIntentHandlerClassName, "isJavascriptSchemeOrInvalidUrl",
                sIntentHandlerClassName);

        changeMethodOwner(
                sIntentHandlerClassName, "extractUrlFromIntent", sBraveIntentHandlerClassName);
    }
}
