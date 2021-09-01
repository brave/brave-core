/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveContentSettingsResourcesClassAdapter extends BraveClassVisitor {
    static String sContentSettingsResourcesClassName = "org/chromium/components/browser_ui/site_settings/ContentSettingsResources";
    static String sBraveContentSettingsResourcesClassName = "org/chromium/components/browser_ui/site_settings/BraveContentSettingsResources";
    static String sContentSettingsResourcesResourceItemClassName = "org/chromium/components/browser_ui/site_settings/ContentSettingsResources$ResourceItem";
    static String sBraveContentSettingsResourcesResourceItemClassName = "org/chromium/components/browser_ui/site_settings/BraveContentSettingsResources$ResourceItem";

    public BraveContentSettingsResourcesClassAdapter(ClassVisitor visitor) {
        super(visitor);

        makePublicMethod(sContentSettingsResourcesClassName, "getResourceItem");
        changeMethodOwner(
                sContentSettingsResourcesClassName, "getResourceItem", sBraveContentSettingsResourcesClassName);
        makePublicInnerClass(sContentSettingsResourcesClassName, "ResourceItem");
        redirectConstructor(sBraveContentSettingsResourcesResourceItemClassName, sContentSettingsResourcesResourceItemClassName);
        redirectTypeInMethod("getResourceItem", sBraveContentSettingsResourcesResourceItemClassName, sContentSettingsResourcesResourceItemClassName);
    }
}
