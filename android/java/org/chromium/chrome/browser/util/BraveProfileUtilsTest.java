/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.util;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import org.junit.After;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import org.robolectric.annotation.Config;

import org.chromium.base.Callback;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.profiles.ProfileManager;

/** Unit tests for {@link BraveProfileUtils}. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class BraveProfileUtilsTest {
    @Rule public final MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private Profile mRegularProfile;
    @Mock private Profile mIncognitoProfile;
    @Mock private Callback<Profile> mCallback;

    @After
    public void tearDown() {
        ProfileManager.resetForTesting();
    }

    @Test
    public void testProfileAlreadyExists_runsImmediately() {
        ProfileManager.setLastUsedProfileForTesting(mRegularProfile);

        BraveProfileUtils.onRegularProfileAvailable(mCallback);

        verify(mCallback).onResult(mRegularProfile);
    }

    @Test
    public void testProfileCreatedLater_runsOnProfileAdded() {
        BraveProfileUtils.onRegularProfileAvailable(mCallback);
        verify(mCallback, never()).onResult(any());

        ProfileManager.onProfileAdded(mRegularProfile);

        verify(mCallback).onResult(mRegularProfile);
    }

    @Test
    public void testIncognitoProfileIgnored() {
        when(mIncognitoProfile.isOffTheRecord()).thenReturn(true);

        BraveProfileUtils.onRegularProfileAvailable(mCallback);
        ProfileManager.onProfileAdded(mIncognitoProfile);
        verify(mCallback, never()).onResult(any());

        ProfileManager.onProfileAdded(mRegularProfile);

        verify(mCallback).onResult(mRegularProfile);
    }

    @Test
    public void testObserverRemovedAfterDelivery() {
        BraveProfileUtils.onRegularProfileAvailable(mCallback);
        ProfileManager.onProfileAdded(mRegularProfile);
        ProfileManager.onProfileAdded(mRegularProfile);

        verify(mCallback, times(1)).onResult(mRegularProfile);
    }
}
