/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveSearchEngineAdapterClassAdapter extends BraveClassVisitor {
    static String sSearchEngineAdapterClassName =
            "org/chromium/chrome/browser/search_engines/settings/SearchEngineAdapter";

    static String sBraveSearchEngineAdapterClassName =
            "org/chromium/chrome/browser/search_engines/settings/BraveSearchEngineAdapter";

    static String sBraveBaseSearchEngineAdapterClassName =
            "org/chromium/chrome/browser/search_engines/settings/BraveBaseSearchEngineAdapter";

    static String sSearchEngineSettingsClassName =
            "org/chromium/chrome/browser/search_engines/settings/SearchEngineSettings";

    static String sBraveSearchEnginePreferenceClassName =
            "org/chromium/chrome/browser/search_engines/settings/BraveSearchEnginePreference";

    static String sMethodGetSearchEngineSourceType = "getSearchEngineSourceType";

    static String sMethodSortAndFilterUnnecessaryTemplateUrl =
            "sortAndFilterUnnecessaryTemplateUrl";

    public BraveSearchEngineAdapterClassAdapter(ClassVisitor visitor) {
        super(visitor);
        changeSuperName(sSearchEngineAdapterClassName, sBraveBaseSearchEngineAdapterClassName);

        changeMethodOwner(sSearchEngineAdapterClassName, sMethodGetSearchEngineSourceType,
                sBraveBaseSearchEngineAdapterClassName);

        changeMethodOwner(sSearchEngineAdapterClassName, sMethodSortAndFilterUnnecessaryTemplateUrl,
                sBraveBaseSearchEngineAdapterClassName);

        deleteField(sBraveSearchEngineAdapterClassName, "mProfile");
        makeProtectedField(sSearchEngineAdapterClassName, "mProfile");
    }
}
