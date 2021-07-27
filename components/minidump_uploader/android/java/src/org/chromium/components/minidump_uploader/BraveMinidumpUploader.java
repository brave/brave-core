/**
 * Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.components.minidump_uploader;

import org.chromium.components.minidump_uploader.MinidumpUploader;
                                        import org.chromium.base.Log;

public class BraveMinidumpUploader extends MinidumpUploader {
    public static final String CRASH_URL_STRING = "https://cr.brave.com";
    public BraveMinidumpUploader() {
        super();
Log.e("TAGAB", "BRAVE BraveMinidumpUploader.ctor (1) 000 CRASH_URL_STRING="+CRASH_URL_STRING);
    }
}
