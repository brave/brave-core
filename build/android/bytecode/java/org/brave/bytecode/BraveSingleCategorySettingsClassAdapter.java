/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveSingleCategorySettingsClassAdapter extends BraveClassVisitor {
    static String sSingleCategorySettingsClassName =
            "org/chromium/components/browser_ui/site_settings/SingleCategorySettings";
    static String sBraveSingleCategorySettingsClassName =
            "org/chromium/components/browser_ui/site_settings/BraveSingleCategorySettings";

    public BraveSingleCategorySettingsClassAdapter(ClassVisitor visitor) {
        super(visitor);

        changeSuperName(sSingleCategorySettingsClassName, sBraveSingleCategorySettingsClassName);
        changeMethodOwner(
                sSingleCategorySettingsClassName,
                "onOptionsItemSelected",
                sBraveSingleCategorySettingsClassName);
        changeMethodOwner(
                sSingleCategorySettingsClassName,
                "getAddExceptionDialogMessageResourceId",
                sBraveSingleCategorySettingsClassName);
        changeMethodOwner(
                sSingleCategorySettingsClassName,
                "resetList",
                sBraveSingleCategorySettingsClassName);
    }
}
