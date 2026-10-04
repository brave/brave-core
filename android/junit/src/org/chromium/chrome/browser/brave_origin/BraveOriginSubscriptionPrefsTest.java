/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.brave_origin;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoInteractions;
import static org.mockito.Mockito.when;

import androidx.test.filters.SmallTest;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.components.prefs.PrefService;
import org.chromium.components.user_prefs.UserPrefs;

/** Unit tests for {@link BraveOriginSubscriptionPrefs}. */
@RunWith(BaseRobolectricTestRunner.class)
public class BraveOriginSubscriptionPrefsTest {
    @Rule public MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private Profile mProfile;
    @Mock private PrefService mPrefService;

    @Before
    public void setUp() {
        UserPrefs.setPrefServiceForTesting(mPrefService);
    }

    @Test
    @SmallTest
    public void isProfileUsable_nullProfile_returnsFalse() {
        assertFalse(BraveOriginSubscriptionPrefs.isProfileUsable(null));
    }

    @Test
    @SmallTest
    public void isProfileUsable_destroyedProfile_returnsFalse() {
        when(mProfile.shutdownStarted()).thenReturn(true);

        assertFalse(BraveOriginSubscriptionPrefs.isProfileUsable(mProfile));
    }

    @Test
    @SmallTest
    public void isProfileUsable_liveProfile_returnsTrue() {
        when(mProfile.shutdownStarted()).thenReturn(false);

        assertTrue(BraveOriginSubscriptionPrefs.isProfileUsable(mProfile));
    }

    /**
     * A profile destroyed while an async Origin flow was in flight must not reach the prefs: its
     * native BrowserContext is gone, so UserPrefs.get() returns null despite being declared
     * non-null. See https://github.com/brave/brave-browser/issues/NNNNN.
     */
    @Test
    @SmallTest
    public void setIsSubscriptionActive_destroyedProfile_doesNotWritePrefs() {
        when(mProfile.shutdownStarted()).thenReturn(true);

        BraveOriginSubscriptionPrefs.setIsSubscriptionActive(mProfile, true);

        verifyNoInteractions(mPrefService);
    }

    @Test
    @SmallTest
    public void setIsSubscriptionActive_nullProfile_doesNotWritePrefs() {
        BraveOriginSubscriptionPrefs.setIsSubscriptionActive(null, true);

        verifyNoInteractions(mPrefService);
    }

    @Test
    @SmallTest
    public void setIsSubscriptionActive_liveProfile_writesPref() {
        when(mProfile.shutdownStarted()).thenReturn(false);

        BraveOriginSubscriptionPrefs.setIsSubscriptionActive(mProfile, true);

        verify(mPrefService).setBoolean(BravePref.BRAVE_ORIGIN_SUBSCRIPTION_ACTIVE_ANDROID, true);
    }

    @Test
    @SmallTest
    public void getIsSubscriptionActive_destroyedProfile_returnsFalseWithoutReadingPrefs() {
        when(mProfile.shutdownStarted()).thenReturn(true);

        assertFalse(BraveOriginSubscriptionPrefs.getIsSubscriptionActive(mProfile));
        verifyNoInteractions(mPrefService);
    }

    /** Puts the prefs in the state a purchase leaves behind before credentials are fetched. */
    private void setUpFetchingState(boolean active, String orderId, String purchaseToken) {
        when(mProfile.shutdownStarted()).thenReturn(false);
        when(mPrefService.getBoolean(BravePref.BRAVE_ORIGIN_SUBSCRIPTION_ACTIVE_ANDROID))
                .thenReturn(active);
        when(mPrefService.getString(BravePref.BRAVE_ORIGIN_ORDER_ID_ANDROID)).thenReturn(orderId);
        when(mPrefService.getString(BravePref.BRAVE_ORIGIN_PURCHASE_TOKEN_ANDROID))
                .thenReturn(purchaseToken);
    }

    /**
     * A failed credential fetch leaves exactly the state a killed one does, because the order ID is
     * only persisted on success. That is why returning to the Origin settings screen has to restart
     * the fetch: without it the screen shows a spinner nothing will ever resolve.
     */
    @Test
    @SmallTest
    public void isFetchingCredentials_activeWithNoOrderId_returnsTrue() {
        setUpFetchingState(/* active= */ true, /* orderId= */ "", /* purchaseToken= */ "token");

        assertTrue(BraveOriginSubscriptionPrefs.isFetchingCredentials(mProfile));
    }

    @Test
    @SmallTest
    public void isFetchingCredentials_orderIdPersisted_returnsFalse() {
        setUpFetchingState(
                /* active= */ true, /* orderId= */ "order", /* purchaseToken= */ "token");

        assertFalse(BraveOriginSubscriptionPrefs.isFetchingCredentials(mProfile));
    }

    @Test
    @SmallTest
    public void isFetchingCredentials_noPurchaseToken_returnsFalse() {
        setUpFetchingState(/* active= */ true, /* orderId= */ "", /* purchaseToken= */ "");

        assertFalse(BraveOriginSubscriptionPrefs.isFetchingCredentials(mProfile));
    }

    @Test
    @SmallTest
    public void isFetchingCredentials_subscriptionInactive_returnsFalse() {
        setUpFetchingState(/* active= */ false, /* orderId= */ "", /* purchaseToken= */ "token");

        assertFalse(BraveOriginSubscriptionPrefs.isFetchingCredentials(mProfile));
    }

    @Test
    @SmallTest
    public void isFetchingCredentials_destroyedProfile_returnsFalse() {
        when(mProfile.shutdownStarted()).thenReturn(true);

        assertFalse(BraveOriginSubscriptionPrefs.isFetchingCredentials(null));
        assertFalse(BraveOriginSubscriptionPrefs.isFetchingCredentials(mProfile));
    }

    /**
     * The settings screen calls this on every entry, so it must stay a no-op once the order ID is
     * persisted - otherwise opening Origin settings would refetch credentials every time.
     */
    @Test
    @SmallTest
    public void resumeCredentialFetchIfNeeded_notFetching_startsNoFetch() {
        setUpFetchingState(
                /* active= */ true, /* orderId= */ "order", /* purchaseToken= */ "token");

        BraveOriginSubscriptionPrefs.resumeCredentialFetchIfNeeded(
                mProfile, /* openSettings= */ false);

        // createFetchOrder() reads these two to build the receipt payload; untouched means it
        // never ran.
        verify(mPrefService, never()).getString(BravePref.BRAVE_ORIGIN_PACKAGE_NAME_ANDROID);
        verify(mPrefService, never()).getString(BravePref.BRAVE_ORIGIN_PRODUCT_ID_ANDROID);
    }

    @Test
    @SmallTest
    public void resumeCredentialFetchIfNeeded_destroyedProfile_startsNoFetch() {
        when(mProfile.shutdownStarted()).thenReturn(true);

        BraveOriginSubscriptionPrefs.resumeCredentialFetchIfNeeded(
                mProfile, /* openSettings= */ false);

        verifyNoInteractions(mPrefService);
    }
}
