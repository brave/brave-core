/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import org.jni_zero.JNINamespace;
import org.jni_zero.NativeMethods;

import org.chromium.build.annotations.NullMarked;
import org.chromium.components.prefs.PrefService;

/** Helper for retrieving a {@link PrefService} from a browser local state. */
@JNINamespace("chrome::android")
@NullMarked
public class BraveLocalState {
    /** Returns the {@link PrefService} associated with the browser's local state */
    public static PrefService get() {
        return BraveLocalStateJni.get().getPrefService();
    }

    /** Makes local state data be written to the disk asap */
    public static void commitPendingWrite() {
        BraveLocalStateJni.get().commitPendingWrite();
    }

    @NativeMethods
    public interface Natives {
        // this method cannot be called 'get', because of the error:
        // method get() is already defined in class BraveLocalStateJni
        PrefService getPrefService();

        void commitPendingWrite();
    }
}
