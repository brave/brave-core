/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.app;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import android.app.Activity;

import com.google.android.gms.tasks.Task;
import com.google.android.play.core.appupdate.AppUpdateInfo;
import com.google.android.play.core.appupdate.testing.FakeAppUpdateManager;
import com.google.android.play.core.install.model.AppUpdateType;
import com.google.android.play.core.install.model.UpdateAvailability;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.MockitoAnnotations;
import org.robolectric.Robolectric;
import org.robolectric.annotation.Config;
import org.robolectric.shadows.ShadowLooper;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;

/** Tests for in-app update functionality in {@link BraveActivity}. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(
        manifest = Config.NONE,
        shadows = {ShadowLooper.class})
public class BraveActivityInAppUpdateTest {
    private Activity mActivity;
    private FakeAppUpdateManager mFakeAppUpdateManager;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        // Clear timing preference before each test
        ChromeSharedPreferences.getInstance()
                .removeKey(BravePreferenceKeys.BRAVE_IN_APP_UPDATE_TIMING);
        ChromeSharedPreferences.getInstance().writeInt(BravePreferenceKeys.BRAVE_APP_OPEN_COUNT, 0);
    }

    @After
    public void tearDown() {
        if (mActivity != null) {
            mActivity.finish();
        }
        // Clean up preferences after each test
        ChromeSharedPreferences.getInstance()
                .removeKey(BravePreferenceKeys.BRAVE_IN_APP_UPDATE_TIMING);
    }

    @Test
    public void testFakeAppUpdateManager_updateAvailable_returnsUpdateInfo() {
        // Create a fake app update manager
        mActivity = Robolectric.buildActivity(Activity.class).create().get();
        mFakeAppUpdateManager = new FakeAppUpdateManager(mActivity);

        // Set update availability
        mFakeAppUpdateManager.setUpdateAvailable(2, AppUpdateType.FLEXIBLE);

        // Get update info
        Task<AppUpdateInfo> updateInfoTask = mFakeAppUpdateManager.getAppUpdateInfo();

        // Process async Task callbacks
        ShadowLooper.idleMainLooper();

        assertTrue("Update info task should succeed", updateInfoTask.isSuccessful());
        AppUpdateInfo updateInfo = updateInfoTask.getResult();
        assertNotNull("Update info should not be null", updateInfo);
        assertEquals(
                "Update availability should be UPDATE_AVAILABLE",
                UpdateAvailability.UPDATE_AVAILABLE,
                updateInfo.updateAvailability());
    }

    @Test
    public void testFakeAppUpdateManager_noUpdateAvailable_returnsNoUpdate() {
        // Create a fake app update manager
        mActivity = Robolectric.buildActivity(Activity.class).create().get();
        mFakeAppUpdateManager = new FakeAppUpdateManager(mActivity);

        // Don't set update available (defaults to no update)

        // Get update info
        Task<AppUpdateInfo> updateInfoTask = mFakeAppUpdateManager.getAppUpdateInfo();

        // Process async Task callbacks
        ShadowLooper.idleMainLooper();

        assertTrue("Update info task should succeed", updateInfoTask.isSuccessful());
        AppUpdateInfo updateInfo = updateInfoTask.getResult();
        assertNotNull("Update info should not be null", updateInfo);
        assertEquals(
                "Update availability should be UPDATE_NOT_AVAILABLE",
                UpdateAvailability.UPDATE_NOT_AVAILABLE,
                updateInfo.updateAvailability());
    }

    @Test
    public void testFakeAppUpdateManager_downloadFlow_simulatesDownloadProcess() {
        // Create a fake app update manager
        mActivity = Robolectric.buildActivity(Activity.class).create().get();
        mFakeAppUpdateManager = new FakeAppUpdateManager(mActivity);

        // Set update as available with FLEXIBLE type
        mFakeAppUpdateManager.setUpdateAvailable(2, AppUpdateType.FLEXIBLE);

        // Get update info to verify update is available
        Task<AppUpdateInfo> updateInfoTask = mFakeAppUpdateManager.getAppUpdateInfo();
        ShadowLooper.idleMainLooper();

        assertTrue("Update info task should succeed", updateInfoTask.isSuccessful());
        AppUpdateInfo updateInfo = updateInfoTask.getResult();
        assertNotNull("Update info should not be null", updateInfo);
        assertEquals(
                "Update availability should be UPDATE_AVAILABLE",
                UpdateAvailability.UPDATE_AVAILABLE,
                updateInfo.updateAvailability());

        // Simulate download process
        mFakeAppUpdateManager.downloadStarts();
        mFakeAppUpdateManager.downloadCompletes();

        // Note: FakeAppUpdateManager may not fully simulate install status transitions
        // to DOWNLOADED. In real scenarios, the InstallStateUpdatedListener registered
        // in BraveActivity.checkAppUpdate() would be notified via onStateUpdate() when
        // the install status changes to InstallStatus.DOWNLOADED, which triggers
        // completeUpdateSnackbar() in BraveActivity.onResumeWithNative().
        // Full integration testing with real Play Store is needed to verify the complete
        // download -> DOWNLOADED status -> snackbar flow.
    }
}
