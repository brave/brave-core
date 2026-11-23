/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.download;

import android.os.Environment;
import android.view.View;

import androidx.test.filters.LargeTest;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.RuleChain;
import org.junit.runner.RunWith;

import org.chromium.base.test.util.Batch;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.CriteriaHelper;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.Features.DisableFeatures;
import org.chromium.base.test.util.Features.EnableFeatures;
import org.chromium.base.test.util.UrlUtils;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.flags.ChromeSwitches;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.transit.AutoResetCtaTransitTestRule;
import org.chromium.chrome.test.transit.ChromeTransitTestRules;
import org.chromium.content_public.browser.test.util.TouchCommon;
import org.chromium.net.test.EmbeddedTestServer;
import org.chromium.ui.base.PageTransition;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;

/**
 * End-to-end coverage of native {@code BraveDownloadManagerDelegate} on Android. Note around
 * ChromeFeatureList.ANDROID_BOTTOM_BAR: This allows to short-circuit the code-path inside {@link
 * org.chromium.chrome.browser.toolbar.bottom.BottomToolbarConfiguration} which checks for small
 * screen. Checking for that results in querying a null {@link Activity} because {@link
 * org.chromium.chrome.browser.document.BraveLauncherActivity} is not yet added to the list of
 * running Activities.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
@EnableFeatures(ChromeFeatureList.ANDROID_BOTTOM_BAR)
@Batch(Batch.PER_CLASS)
public class BraveDownloadImageMetadataStripperTest {
    static final String STRIP_IMAGE_METADATA_FEATURE = "StripImageMetadataV1";

    // The "real" JPEG fixture shared with the desktop browser test. It carries a Facebook FBMD
    // record inside its IPTC metadata that the stripper is expected to scrub.
    private static final String TEST_IMAGE_FILE_NAME = "fbmd_test_image.jpg";

    private static final String TEST_DATA_DIR = "brave/test/data/image_metadata_stripper/";

    private static final String DOWNLOAD_PAGE_PATH =
            "/" + TEST_DATA_DIR + "fbmd_download_test.html";

    // The ASCII marker that begins the Facebook FBMD IPTC record.
    private static final byte[] FBMD_MARKER = "FBMD".getBytes(StandardCharsets.US_ASCII);

    private static final File DOWNLOAD_DIRECTORY =
            Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS);

    public final AutoResetCtaTransitTestRule mActivityTestRule =
            ChromeTransitTestRules.fastAutoResetCtaActivityRule();

    public final DownloadTestRule mDownloadTestRule = new DownloadTestRule();

    @Rule
    public final RuleChain mRuleChain =
            RuleChain.outerRule(mActivityTestRule).around(mDownloadTestRule);

    private EmbeddedTestServer mTestServer;

    /** Navigates to the download page, clicks the link and waits for the download to complete. */
    private void downloadTestImage() {
        mActivityTestRule.loadUrlInTab(
                mTestServer.getURL(DOWNLOAD_PAGE_PATH),
                PageTransition.TYPED | PageTransition.FROM_ADDRESS_BAR,
                mActivityTestRule.getActivityTab());
        waitForFocus();

        int callCount = mDownloadTestRule.getChromeDownloadCallCount();
        TouchCommon.singleClickView(mActivityTestRule.getActivityTab().getView());
        Assert.assertTrue(
                "Timed out waiting for the download to finish",
                mDownloadTestRule.waitForChromeDownloadToFinish(callCount));
        Assert.assertTrue(
                "The image should always finish downloading",
                mDownloadTestRule.hasDownloaded(TEST_IMAGE_FILE_NAME, null));
    }

    /** Reads the bytes of the downloaded image from the system downloads directory. */
    private byte[] readDownloadedImageBytes() throws IOException {
        return readAllBytes(new File(DOWNLOAD_DIRECTORY, TEST_IMAGE_FILE_NAME));
    }

    /** Reads the original fixture bytes from the on-device test data root. */
    private byte[] readOriginalImageBytes() throws IOException {
        return readAllBytes(
                new File(UrlUtils.getIsolatedTestFilePath(TEST_DATA_DIR + TEST_IMAGE_FILE_NAME)));
    }

    /** Whether the given bytes still contain the Facebook FBMD IPTC marker. */
    private static boolean containsFbmd(byte[] data) {
        outer:
        for (int i = 0; i + FBMD_MARKER.length <= data.length; i++) {
            for (int j = 0; j < FBMD_MARKER.length; j++) {
                if (data[i + j] != FBMD_MARKER[j]) {
                    continue outer;
                }
            }
            return true;
        }
        return false;
    }

    private static byte[] readAllBytes(File file) throws IOException {
        try (FileInputStream in = new FileInputStream(file);
                ByteArrayOutputStream out = new ByteArrayOutputStream()) {
            byte[] buffer = new byte[4096];
            int read;
            while ((read = in.read(buffer)) != -1) {
                out.write(buffer, 0, read);
            }
            return out.toByteArray();
        }
    }

    private void waitForFocus() {
        View currentView = mActivityTestRule.getActivityTab().getView();
        if (!currentView.hasFocus()) {
            TouchCommon.singleClickView(currentView);
        }
        CriteriaHelper.pollUiThread(
                currentView::hasFocus, "Tab view never gained focus before clicking download link");
    }

    @Before
    public void setUp() {
        mActivityTestRule.startOnBlankPage();
        mDownloadTestRule.attach(mActivityTestRule.getActivity());
        mTestServer = mActivityTestRule.getTestServer();
        mDownloadTestRule.resetCallbackHelper();
    }

    @After
    public void tearDown() {
        mDownloadTestRule.deleteFilesInDownloadDirectory(TEST_IMAGE_FILE_NAME);
    }

    @Test
    @LargeTest
    @Feature({"Downloads"})
    @EnableFeatures(STRIP_IMAGE_METADATA_FEATURE)
    public void testRemovesFbmdMetadataFromDownloadedJpegImage() throws Exception {
        byte[] originalContents = readOriginalImageBytes();
        // Sanity check the fixture actually carries an FBMD record to strip.
        Assert.assertTrue(
                "Test fixture should carry an FBMD record", containsFbmd(originalContents));

        downloadTestImage();

        byte[] downloadedContents = readDownloadedImageBytes();
        Assert.assertFalse(
                "FBMD metadata should have been stripped from the download",
                containsFbmd(downloadedContents));
        Assert.assertEquals(
                "Stripping should not change the file size",
                originalContents.length,
                downloadedContents.length);
    }

    @Test
    @LargeTest
    @Feature({"Downloads"})
    @DisableFeatures(STRIP_IMAGE_METADATA_FEATURE)
    public void testDoesNotStripMetadataWhenFeatureDisabled() throws Exception {
        downloadTestImage();

        byte[] downloadedContents = readDownloadedImageBytes();
        Assert.assertTrue(
                "FBMD metadata should be preserved when the feature is disabled",
                containsFbmd(downloadedContents));
    }
}
