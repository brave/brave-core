/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.playlist.hls_content;

import org.chromium.build.annotations.IdentifierNameString;
import org.chromium.chrome.browser.base.SplitCompatService;

/** See {@link HlsServiceImpl}. */
public class HlsService extends SplitCompatService {
    @IdentifierNameString
    private static final String sImplClassName =
            "org.chromium.chrome.browser.playlist.hls_content.HlsServiceImpl";

    public HlsService() {
        super(sImplClassName);
    }
}
