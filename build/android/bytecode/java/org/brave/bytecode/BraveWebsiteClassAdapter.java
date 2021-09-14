/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveWebsiteClassAdapter extends BraveClassVisitor {
    static String sWebsiteClassName = "org/chromium/components/browser_ui/site_settings/Website";
    static String sBraveWebsiteClassName = "org/chromium/components/browser_ui/site_settings/BraveWebsite";

    public BraveWebsiteClassAdapter(ClassVisitor visitor) {
        super(visitor);

        changeSuperName(sBraveWebsiteClassName, sWebsiteClassName);
        makeNonFinalClass(sWebsiteClassName);

        makePublicMethod(sWebsiteClassName, "setContentSetting");
        redirectConstructor(sWebsiteClassName, sBraveWebsiteClassName);

//         changeMethodOwner(sWebsiteClassName, "setContentSetting", sBraveWebsiteClassName);
//         changeMethodOwner(sBraveWebsiteClassName, "getAddress", sWebsiteClassName);
//         changeMethodOwner(sBraveWebsiteClassName, "getPermissionInfo", sWebsiteClassName);
//         changeMethodOwner(sBraveWebsiteClassName, "getContentSettingException", sWebsiteClassName);
//         changeMethodOwner(sBraveWebsiteClassName, "setContentSettingException", sWebsiteClassName);
        
        deleteMethod(sBraveWebsiteClassName, "getAddress");
        deleteMethod(sBraveWebsiteClassName, "getPermissionInfo");
        deleteMethod(sBraveWebsiteClassName, "getContentSettingException");
        deleteMethod(sBraveWebsiteClassName, "setContentSettingException");
    }
}
