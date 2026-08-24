/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.brave_origin;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
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
}
