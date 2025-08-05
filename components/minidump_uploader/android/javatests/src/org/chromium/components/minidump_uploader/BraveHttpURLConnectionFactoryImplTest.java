/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.components.minidump_uploader;

import android.net.Uri;

import androidx.test.filters.SmallTest;

import org.junit.Assert;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.annotation.Config;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.base.version_info.VersionInfo;
import org.chromium.components.minidump_uploader.util.BraveHttpURLConnectionFactoryImpl;
import org.chromium.components.minidump_uploader.util.HttpURLConnectionFactory;

import java.net.HttpURLConnection;

/** Unittests for {@link BraveHttpURLConnectionFactoryImpl}. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class BraveHttpURLConnectionFactoryImplTest {
    @Test
    @SmallTest
    public void testUploadUrlHasProductVersionGuid() {
        HttpURLConnectionFactory httpURLConnectionFactory = new BraveHttpURLConnectionFactoryImpl();
        HttpURLConnection connection = httpURLConnectionFactory.createHttpURLConnection("");
        Uri uri = Uri.parse(connection.getURL().toString());
        Assert.assertEquals("Brave_Android", uri.getQueryParameter("product"));
        Assert.assertEquals(VersionInfo.getProductVersion(), uri.getQueryParameter("version"));
        Assert.assertEquals("00000000-0000-0000-0000-000000000000", uri.getQueryParameter("guid"));
    }
}
