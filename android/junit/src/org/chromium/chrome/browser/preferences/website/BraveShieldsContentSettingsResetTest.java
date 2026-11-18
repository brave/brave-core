/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.preferences.website;

import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoMoreInteractions;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.browser.profiles.Profile;

/** Unit tests for {@link BraveShieldsContentSettings#resetSiteToDefaults}. */
@RunWith(BaseRobolectricTestRunner.class)
public class BraveShieldsContentSettingsResetTest {
    @Rule public final MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private Profile mProfile;
    @Mock private BraveShieldsContentSettings.Natives mNatives;

    private static final String SITE_URL = "https://example.com";

    @Before
    public void setUp() {
        BraveShieldsContentSettingsJni.setInstanceForTesting(mNatives);
    }

    @After
    public void tearDown() {
        BraveShieldsContentSettingsJni.setInstanceForTesting(null);
    }

    @Test
    public void testResetSiteToDefaults_delegatesToNativeReset() {
        BraveShieldsContentSettings.resetSiteToDefaults(mProfile, SITE_URL);

        // The actual per-setting clearing lives natively (ResetShieldsSettingsForSite /
        // ResetForgetFirstPartyStorageEnabled) so the site keeps following the global defaults
        // as they change later, instead of this layer snapshotting current values per setting.
        verify(mNatives).resetSiteToDefaults(SITE_URL, mProfile);
        verifyNoMoreInteractions(mNatives);
    }
}
