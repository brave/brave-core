/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.components.minidump_uploader;

import androidx.test.filters.SmallTest;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.annotation.Config;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.components.minidump_uploader.util.HttpURLConnectionFactory;

import java.io.File;
import java.io.IOException;
import java.net.HttpURLConnection;
import java.net.URL;

/** Unittest for {@link MinidumpUploader}. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class BraveMinidumpUploaderTest {
    @Rule public CrashTestRule mTestRule = new CrashTestRule();
    private File mUploadTestFile;

    // Returns a fixed HTTP error code, without overriding the URL — used to verify
    // that upload() reaches the HTTP layer under Brave's real sCrashUrlString value.
    private static class ErrorCodeHttpURLConnectionFactory implements HttpURLConnectionFactory {
        private final int mErrorCode;

        ErrorCodeHttpURLConnectionFactory(int errorCode) {
            mErrorCode = errorCode;
        }

        @Override
        public HttpURLConnection createHttpURLConnection(String url) {
            try {
                return new TestHttpURLConnection(new URL(url)) {
                    @Override
                    public int getResponseCode() {
                        return mErrorCode;
                    }
                };
            } catch (IOException e) {
                return null;
            }
        }
    }

    @Before
    public void setUp() throws IOException {
        // Do NOT call setCrashUrlStringForTesting — we intentionally exercise the real
        // Brave-patched value so that a future upstream change that nulls the field (or
        // adds a new precondition) turns into a red test rather than a silent breakage.
        mUploadTestFile = new File(mTestRule.getCrashDir(), "crashFile");
        CrashTestRule.setUpMinidumpFile(mUploadTestFile, MinidumpUploaderTestConstants.BOUNDARY);
    }

    @After
    public void tearDown() {
        mUploadTestFile.delete();
    }

    @Test
    @SmallTest
    public void testCrashUrlStringIsBraveEndpoint() {
        Assert.assertEquals("https://cr.brave.com", MinidumpUploader.sCrashUrlString);
    }

    @Test
    @SmallTest
    public void testUploadReachesHttpLayer() {
        // If sCrashUrlString is null/empty, or upstream adds a new gate inside upload(),
        // this will return isFailure() instead of isUploadError() and the test will fail.
        MinidumpUploader uploader =
                new MinidumpUploader(new ErrorCodeHttpURLConnectionFactory(500));
        MinidumpUploader.Result result = uploader.upload(mUploadTestFile);
        Assert.assertTrue(result.isUploadError());
        Assert.assertEquals(500, result.errorCode());
    }
}
