/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class StackTabListSceneLayerClassAdapter extends BraveClassVisitor {
    static String sStackTabListSceneLayerClassName =
            "org/chromium/chrome/browser/compositor/scene_layer/StackTabListSceneLayer";

    static String sTabListSceneLayerClassName =
            "org/chromium/chrome/browser/compositor/scene_layer/TabListSceneLayer";

    public StackTabListSceneLayerClassAdapter(ClassVisitor visitor) {
        super(visitor);

        deleteField(sStackTabListSceneLayerClassName, "mNativePtr");
        makeProtectedField(sTabListSceneLayerClassName, "mNativePtr");

        deleteField(sStackTabListSceneLayerClassName, "mIsInitialized");
        makeProtectedField(sTabListSceneLayerClassName, "mIsInitialized");
    }
}
