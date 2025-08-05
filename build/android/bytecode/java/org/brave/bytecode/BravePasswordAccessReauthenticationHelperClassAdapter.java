/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

/**
 * Adapter, used to make some fields from upstream's PasswordAccessReauthenticationHelper be public
 */
public class BravePasswordAccessReauthenticationHelperClassAdapter extends BraveClassVisitor {
    // TODO(alexeybarabash): backport PasswordAccessReauthenticationHelper
    // Upstream change: 6875d2ce472e7f2097617525feec45313602e225
    // static String sHelperClassName =
    //
    // "org/chromium/chrome/browser/password_manager/settings/PasswordAccessReauthenticationHelper";
    // // presubmit: ignore-long-line
    // static String sBraveHelperClassName =
    //
    // "org/chromium/chrome/browser/sync/settings/BravePasswordAccessReauthenticationHelper";

    BravePasswordAccessReauthenticationHelperClassAdapter(ClassVisitor visitor) {
        super(visitor);

        // deleteField(sBraveHelperClassName, "mCallback");
        // makeProtectedField(sHelperClassName, "mCallback");

        // deleteField(sBraveHelperClassName, "mFragmentManager");
        // makeProtectedField(sHelperClassName, "mFragmentManager");
    }
}
