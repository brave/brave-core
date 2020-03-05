// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveClassAdapter {
    public static ClassVisitor createAdapter(ClassVisitor chain) {
        chain = new BraveBookmarkModelClassAdapter(chain);
        chain = new BraveMainPreferenceBaseClassAdapter(chain);
        return chain;
    }
}
