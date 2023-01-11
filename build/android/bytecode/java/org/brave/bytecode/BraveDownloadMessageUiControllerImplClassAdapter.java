/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveDownloadMessageUiControllerImplClassAdapter extends BraveClassVisitor {
    static String sDownloadMessageUiControllerImpl =
            "org/chromium/chrome/browser/download/DownloadMessageUiControllerImpl";

    static String sBraveDownloadMessageUiControllerImpl =
            "org/chromium/chrome/browser/download/BraveDownloadMessageUiControllerImpl";

    public BraveDownloadMessageUiControllerImplClassAdapter(ClassVisitor visitor) {
        super(visitor);

        changeSuperName(sDownloadMessageUiControllerImpl, sBraveDownloadMessageUiControllerImpl);
        changeMethodOwner(sDownloadMessageUiControllerImpl, "isVisibleToUser",
                sBraveDownloadMessageUiControllerImpl);
    }
}
