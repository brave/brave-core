/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.rate;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import android.app.Dialog;
import android.widget.Button;

import androidx.fragment.app.FragmentActivity;

import com.google.android.gms.tasks.Task;
import com.google.android.play.core.review.ReviewInfo;
import com.google.android.play.core.review.testing.FakeReviewManager;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.MockitoAnnotations;
import org.robolectric.Robolectric;
import org.robolectric.annotation.Config;
import org.robolectric.shadows.ShadowLooper;

import org.chromium.base.test.BaseRobolectricTestRunner;

/** Tests for {@link BraveAskPlayStoreRatingDialog} using FakeReviewManager. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(
        manifest = Config.NONE,
        // ShadowLooper allows manual control of the main thread's message queue, enabling
        // synchronous execution of async operations (like Task callbacks) in tests via
        // ShadowLooper.idleMainLooper().
        shadows = {ShadowLooper.class})
public class BraveAskPlayStoreRatingDialogTest {
    private FragmentActivity mActivity;
    private FakeReviewManager mFakeReviewManager;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mActivity = Robolectric.buildActivity(FragmentActivity.class).create().get();
        mFakeReviewManager = new FakeReviewManager(mActivity);
    }

    @After
    public void tearDown() {
        if (mActivity != null) {
            mActivity.finish();
        }
    }

    @Test
    public void testNewInstance_fromSettings() {
        BraveAskPlayStoreRatingDialog fragment = BraveAskPlayStoreRatingDialog.newInstance(true);
        assertNotNull(fragment);
        assertNotNull(fragment.getArguments());
        assertTrue(fragment.getArguments().getBoolean(RateUtils.FROM_SETTINGS));
    }

    @Test
    public void testNewInstance_notFromSettings() {
        BraveAskPlayStoreRatingDialog fragment = BraveAskPlayStoreRatingDialog.newInstance(false);
        assertNotNull(fragment);
        assertNotNull(fragment.getArguments());
        assertFalse(fragment.getArguments().getBoolean(RateUtils.FROM_SETTINGS));
    }

    @Test
    public void testRequestReviewFlow_withFakeReviewManager_succeeds() {
        BraveAskPlayStoreRatingDialog fragment = BraveAskPlayStoreRatingDialog.newInstance(false);
        assertNotNull("Fragment should be created", fragment);
        fragment.setReviewManagerForTesting(mFakeReviewManager);

        mActivity.getSupportFragmentManager().beginTransaction().add(fragment, null).commitNow();

        // Create dialog to trigger setupDialog
        Dialog dialog = fragment.onCreateDialog(null);
        assertNotNull("Dialog should be created", dialog);
        fragment.setupDialog(dialog, 0);

        // Process async Task callbacks from requestReviewFlow
        ShadowLooper.idleMainLooper();

        // Test passes if no exceptions were thrown, verifying the flow completed successfully
    }

    @Test
    public void testLaunchReviewFlow_withFakeReviewManager_succeeds() {
        BraveAskPlayStoreRatingDialog fragment = BraveAskPlayStoreRatingDialog.newInstance(false);
        assertNotNull("Fragment should be created", fragment);
        fragment.setReviewManagerForTesting(mFakeReviewManager);

        // Properly attach fragment to FragmentManager
        mActivity.getSupportFragmentManager().beginTransaction().add(fragment, null).commitNow();

        // Create dialog to trigger setupDialog
        Dialog dialog = fragment.onCreateDialog(null);
        assertNotNull("Dialog should be created", dialog);
        fragment.setupDialog(dialog, 0);

        // Process async Task callbacks from requestReviewFlow to ensure mReviewInfo is set
        ShadowLooper.idleMainLooper();

        // Find the "Rate Now" button from the dialog's content view
        Button rateNowButton = dialog.findViewById(org.chromium.chrome.R.id.rate_now_button);
        assertNotNull("Rate Now button should exist", rateNowButton);

        // Simulate clicking the button to trigger launchReviewFlow()
        rateNowButton.performClick();

        // Process async Task callbacks from launchReviewFlow
        ShadowLooper.idleMainLooper();

        // Test passes if no exceptions were thrown, verifying the flow completed successfully
    }

    @Test
    public void testFakeReviewManager_requestReviewFlow_alwaysSucceeds() {
        // Test that FakeReviewManager.requestReviewFlow() always succeeds
        Task<ReviewInfo> request = mFakeReviewManager.requestReviewFlow();

        // Process async Task callbacks to ensure tasks execute
        ShadowLooper.idleMainLooper();

        assertTrue(
                "FakeReviewManager.requestReviewFlow() should always succeed",
                request.isSuccessful());
        assertNotNull("ReviewInfo should not be null", request.getResult());
    }

    @Test
    public void testFakeReviewManager_launchReviewFlow_alwaysSucceeds() {
        // Test that FakeReviewManager.launchReviewFlow() always succeeds

        // First request the review flow
        Task<ReviewInfo> request = mFakeReviewManager.requestReviewFlow();
        // Process async Task callbacks
        ShadowLooper.idleMainLooper();

        assertTrue("Request should succeed", request.isSuccessful());
        ReviewInfo reviewInfo = request.getResult();
        assertNotNull("ReviewInfo should not be null", reviewInfo);

        // Then launch the review flow
        Task<Void> flow = mFakeReviewManager.launchReviewFlow(mActivity, reviewInfo);
        // Process async Task callbacks
        ShadowLooper.idleMainLooper();

        assertTrue(
                "FakeReviewManager.launchReviewFlow() should always succeed", flow.isSuccessful());
    }

    @Test
    public void testFragment_withFakeReviewManager_handlesMultipleRequests() {
        // Test that the fragment can handle multiple review flow requests
        BraveAskPlayStoreRatingDialog fragment1 = BraveAskPlayStoreRatingDialog.newInstance(false);
        assertNotNull("Fragment1 should be created", fragment1);
        fragment1.setReviewManagerForTesting(mFakeReviewManager);

        mActivity.getSupportFragmentManager().beginTransaction().add(fragment1, null).commitNow();

        BraveAskPlayStoreRatingDialog fragment2 = BraveAskPlayStoreRatingDialog.newInstance(false);
        assertNotNull("Fragment2 should be created", fragment2);
        fragment2.setReviewManagerForTesting(mFakeReviewManager);

        mActivity.getSupportFragmentManager().beginTransaction().add(fragment2, null).commitNow();

        Dialog dialog1 = fragment1.onCreateDialog(null);
        assertNotNull("Dialog1 should be created", dialog1);
        fragment1.setupDialog(dialog1, 0);

        Dialog dialog2 = fragment2.onCreateDialog(null);
        assertNotNull("Dialog2 should be created", dialog2);
        fragment2.setupDialog(dialog2, 0);

        // Process async Task callbacks from both fragments
        ShadowLooper.idleMainLooper();

        // Test passes if no exceptions were thrown
    }
}
