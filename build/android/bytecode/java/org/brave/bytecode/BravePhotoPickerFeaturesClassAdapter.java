/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BravePhotoPickerFeaturesClassAdapter extends BraveClassVisitor {
    static String sPhotoPickerFeaturesClassName =
            "org/chromium/components/browser_ui/photo_picker/PhotoPickerFeatures";
    static String sBravePhotoPickerFeaturesClassName =
            "org/chromium/components/browser_ui/photo_picker/BravePhotoPickerFeatures";

    public BravePhotoPickerFeaturesClassAdapter(ClassVisitor visitor) {
        super(visitor);

        changeMethodOwner(
                sPhotoPickerFeaturesClassName,
                "launchViaActionGetContent",
                sBravePhotoPickerFeaturesClassName);
    }
}
