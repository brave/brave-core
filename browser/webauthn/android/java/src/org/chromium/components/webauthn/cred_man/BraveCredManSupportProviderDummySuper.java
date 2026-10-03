/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.components.webauthn.cred_man;

import org.chromium.build.annotations.NullMarked;

/**
 * Holds a same-named stub that bytecode-redirects to the private {@code
 * CredManSupportProvider#hasOldGmsVersion}, so {@link BraveCredManSupportProvider} can back-call
 * it despite Java visibility rules. The stub body is never executed: {@code
 * BraveCredManSupportProviderClassAdapter} rewrites the call site to invoke the upstream method
 * (which it also bumps to public at bytecode time).
 */
@NullMarked
class BraveCredManSupportProviderDummySuper extends CredManSupportProvider {
    private BraveCredManSupportProviderDummySuper() {}

    public static boolean hasOldGmsVersion() {
        assert false : "This class usage should be removed via bytecode modification!";
        return false;
    }
}
