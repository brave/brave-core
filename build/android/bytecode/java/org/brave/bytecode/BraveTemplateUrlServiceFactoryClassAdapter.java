/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveTemplateUrlServiceFactoryClassAdapter extends BraveClassVisitor {
    static String sTemplateUrlServiceFactory =
            "org/chromium/chrome/browser/search_engines/TemplateUrlServiceFactory";

    static String sBraveTemplateUrlServiceFactory =
            "org/chromium/chrome/browser/search_engines/BraveTemplateUrlServiceFactory";

    public BraveTemplateUrlServiceFactoryClassAdapter(ClassVisitor visitor) {
        super(visitor);

        changeMethodOwner(sTemplateUrlServiceFactory, "get", sBraveTemplateUrlServiceFactory);
    }
}
