/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveSearchEnginePreferenceClassAdapter extends BraveClassVisitor {
    static String sSearchEngineSettingsClassName =
            "org/chromium/chrome/browser/search_engines/settings/SearchEngineSettings";

    static String sBraveSearchEnginePreferenceClassName =
            "org/chromium/chrome/browser/search_engines/settings/BraveSearchEnginePreference";

    public BraveSearchEnginePreferenceClassAdapter(ClassVisitor visitor) {
        super(visitor);

        deleteField(sBraveSearchEnginePreferenceClassName, "mSearchEngineAdapter");
        makeProtectedField(sSearchEngineSettingsClassName, "mSearchEngineAdapter");

        deleteField(sBraveSearchEnginePreferenceClassName, "mProfile");
        makeProtectedField(sSearchEngineSettingsClassName, "mProfile");

        makePublicMethod(sSearchEngineSettingsClassName, "createAdapterIfNecessary");
        addMethodAnnotation(sBraveSearchEnginePreferenceClassName, "createAdapterIfNecessary",
                "Ljava/lang/Override;");
    }
}
