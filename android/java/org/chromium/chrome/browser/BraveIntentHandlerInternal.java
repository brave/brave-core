/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;

/**
 * Holds same-named stubs that bytecode-redirect to private members of {@link IntentHandler}, so
 * {@link BraveIntentHandler} can back-call them despite Java visibility rules. The stub bodies are
 * never executed: {@code BraveIntentHandlerClassAdapter} rewrites each call site to invoke the
 * upstream method (which it also bumps to public at bytecode time).
 */
@NullMarked
final class BraveIntentHandlerInternal {
    private BraveIntentHandlerInternal() {}

    static boolean isUrlUnsafe(@Nullable String url) {
        assert false;
        return false;
    }
}
