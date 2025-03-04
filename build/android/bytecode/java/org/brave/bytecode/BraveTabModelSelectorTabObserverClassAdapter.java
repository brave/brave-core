/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveTabModelSelectorTabObserverClassAdapter extends BraveClassVisitor {
    static String sTabModelSelectorTabObserverClassName = "org/chromium/chrome/browser/tabmodel/TabModelSelectorTabObserver";
    static String sBraveTabModelSelectorTabObserverClassName = "org/chromium/chrome/browser/tabmodel/BraveTabModelSelectorTabObserver";

    public BraveTabModelSelectorTabObserverClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(sTabModelSelectorTabObserverClassName, sBraveTabModelSelectorTabObserverClassName);
    }
}
