/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BravePreferenceFragmentClassAdapter extends BraveClassVisitor {
    static String sDeveloperSettingsClassName =
            "org/chromium/chrome/browser/tracing/settings/DeveloperSettings";

    static String sBravePreferenceFragmentClassName =
            "org/chromium/chrome/browser/settings/BravePreferenceFragment";

    public BravePreferenceFragmentClassAdapter(ClassVisitor visitor) {
        super(visitor);

        changeSuperName(sDeveloperSettingsClassName, sBravePreferenceFragmentClassName);
    }
}
