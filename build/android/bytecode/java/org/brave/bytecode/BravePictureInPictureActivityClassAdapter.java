/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BravePictureInPictureActivityClassAdapter extends BraveClassVisitor {
    static String sPictureInPictureActivityClassName =
            "org/chromium/chrome/browser/media/PictureInPictureActivity";
    static String sBravePictureInPictureActivityClassName =
            "org/chromium/chrome/browser/media/BravePictureInPictureActivity";

    public BravePictureInPictureActivityClassAdapter(ClassVisitor visitor) {
        super(visitor);

        changeSuperName(
                sPictureInPictureActivityClassName, sBravePictureInPictureActivityClassName);
    }
}
