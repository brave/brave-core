/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.base.version_info;

import org.junit.Assert;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.annotation.Config;

import org.chromium.base.test.BaseRobolectricTestRunner;

/** Unit tests for {@link VersionInfo}, the Java counterpart of version_info_unittest.cc. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class BraveVersionInfoTest {
    @Test
    public void testProductVersionKeepsChromiumMajor() {
        // Consumers like the crash uploader and the sponsored rich media expect
        // 4 segments with the Chromium major code, like 156.1.98.18, not the
        // 3 segments Brave version stored at the manifest.
        String version = VersionInfo.getProductVersion();
        String[] segments = version.split("\\.");
        Assert.assertEquals("Unexpected version " + version, 4, segments.length);
        Assert.assertEquals(
                "Unexpected version " + version,
                VersionConstants.PRODUCT_MAJOR_VERSION,
                Integer.parseInt(segments[0]));
    }
}
