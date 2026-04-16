/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.preferences.website;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

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
    public void testResetSiteToDefaults_appliesAllGlobalDefaultsToSite() {
        when(mNatives.getBraveShieldsEnabled("", mProfile)).thenReturn(true);
        when(mNatives.getNoScriptControlType("", mProfile)).thenReturn("block");
        when(mNatives.getForgetFirstPartyStorageEnabled("", mProfile)).thenReturn(false);
        when(mNatives.getFingerprintingControlType("", mProfile)).thenReturn("block");
        when(mNatives.getHttpsUpgradeControlType("", mProfile)).thenReturn("block");
        when(mNatives.getCookieControlType("", mProfile)).thenReturn("block_third_party");
        when(mNatives.getCosmeticFilteringControlType("", mProfile))
                .thenReturn("block_third_party");
        when(mNatives.getAdControlType("", mProfile)).thenReturn("block");

        BraveShieldsContentSettings.resetSiteToDefaults(mProfile, SITE_URL);

        verify(mNatives).setBraveShieldsEnabled(true, SITE_URL, mProfile);
        verify(mNatives).setNoScriptControlType("block", SITE_URL, mProfile);
        verify(mNatives).setForgetFirstPartyStorageEnabled(false, SITE_URL, mProfile);
        verify(mNatives).setFingerprintingControlType("block", SITE_URL, mProfile);
        verify(mNatives).setHttpsUpgradeControlType("block", SITE_URL, mProfile);
        verify(mNatives).setCookieControlType("block_third_party", SITE_URL, mProfile);
        verify(mNatives).setCosmeticFilteringControlType("block_third_party", SITE_URL, mProfile);
        verify(mNatives).setAdControlType("block", SITE_URL, mProfile);
    }

    @Test
    public void testResetSiteToDefaults_skipsHttpsUpgradeWhenDefaultIsDefault() {
        when(mNatives.getBraveShieldsEnabled("", mProfile)).thenReturn(true);
        when(mNatives.getNoScriptControlType("", mProfile)).thenReturn("block");
        when(mNatives.getForgetFirstPartyStorageEnabled("", mProfile)).thenReturn(false);
        when(mNatives.getFingerprintingControlType("", mProfile)).thenReturn("default");
        when(mNatives.getHttpsUpgradeControlType("", mProfile)).thenReturn("default");
        when(mNatives.getCookieControlType("", mProfile)).thenReturn("block_third_party");
        when(mNatives.getCosmeticFilteringControlType("", mProfile))
                .thenReturn("block_third_party");
        when(mNatives.getAdControlType("", mProfile)).thenReturn("block");

        BraveShieldsContentSettings.resetSiteToDefaults(mProfile, SITE_URL);

        verify(mNatives, never())
                .setHttpsUpgradeControlType(anyString(), anyString(), any(Profile.class));
        verify(mNatives).setBraveShieldsEnabled(true, SITE_URL, mProfile);
        verify(mNatives).setFingerprintingControlType("default", SITE_URL, mProfile);
    }

    @Test
    public void testResetSiteToDefaults_setsHttpsUpgradeWhenDefaultIsAllow() {
        when(mNatives.getBraveShieldsEnabled("", mProfile)).thenReturn(false);
        when(mNatives.getNoScriptControlType("", mProfile)).thenReturn("allow");
        when(mNatives.getForgetFirstPartyStorageEnabled("", mProfile)).thenReturn(true);
        when(mNatives.getFingerprintingControlType("", mProfile)).thenReturn("allow");
        when(mNatives.getHttpsUpgradeControlType("", mProfile)).thenReturn("allow");
        when(mNatives.getCookieControlType("", mProfile)).thenReturn("allow");
        when(mNatives.getCosmeticFilteringControlType("", mProfile)).thenReturn("allow");
        when(mNatives.getAdControlType("", mProfile)).thenReturn("allow");

        BraveShieldsContentSettings.resetSiteToDefaults(mProfile, SITE_URL);

        verify(mNatives).setHttpsUpgradeControlType("allow", SITE_URL, mProfile);
        verify(mNatives).setBraveShieldsEnabled(false, SITE_URL, mProfile);
        verify(mNatives).setForgetFirstPartyStorageEnabled(true, SITE_URL, mProfile);
    }
}
