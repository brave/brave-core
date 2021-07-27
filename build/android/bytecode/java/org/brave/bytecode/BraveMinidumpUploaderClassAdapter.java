/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;


public class BraveMinidumpUploaderClassAdapter extends BraveClassVisitor {
    static String sMinidumpUploaderClassName = "org/chromium/components/minidump_uploader/MinidumpUploader";

    static String sBraveMinidumpUploaderClassName = "org/chromium/components/minidump_uploader/BraveMinidumpUploader";

    public BraveMinidumpUploaderClassAdapter(ClassVisitor visitor) {
        super(visitor);
        //deleteField(sBraveMinidumpUploaderClassName, "CRASH_URL_STRING");
        //makeProtectedField(sMinidumpUploaderClassName, "CRASH_URL_STRING");

        deleteField(sMinidumpUploaderClassName, "CRASH_URL_STRING");
        makeProtectedField(sBraveMinidumpUploaderClassName, "CRASH_URL_STRING");



        // deleteField(sBraveEditUrlSuggestionProcessor, "mHasClearedOmniboxForFocus");
        // makeProtectedField(sEditUrlSuggestionProcessor, "mHasClearedOmniboxForFocus");
    }
}
