/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveClassAdapter {
    public static ClassVisitor createAdapter(ClassVisitor chain) {
        chain = new BraveBookmarkModelClassAdapter(chain);
        chain = new BraveMainPreferenceBaseClassAdapter(chain);
        chain = new BraveAndroidSyncSettingsClassAdapter(chain);
        return chain;
    }
}
