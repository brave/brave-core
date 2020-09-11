/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import android.support.test.filters.SmallTest;
import android.view.View;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.ChromeActivity;
import org.chromium.chrome.browser.ntp.BraveDuckDuckGoOfferView;
import org.chromium.chrome.browser.ntp.IncognitoNewTabPage;
import org.chromium.chrome.browser.onboarding.OnboardingPrefManager;
import org.chromium.chrome.browser.preferences.SharedPreferencesManager;
import org.chromium.chrome.browser.settings.BraveSearchEngineUtils;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.components.search_engines.TemplateUrl;
import org.chromium.content_public.browser.test.util.TestThreadUtils;

/**
 * Brave specific tests for private tab
 */
@RunWith(ChromeJUnit4ClassRunner.class)
public class BravePrivateTabTest {
    @Rule
    public ChromeActivityTestRule<ChromeActivity> mActivityTestRule = new ChromeActivityTestRule<>(
            ChromeActivity.class);

    @Before
    public void setUp() throws Exception {
        // Disable key checker to avoid asserts on Brave keys in debug
        SharedPreferencesManager.getInstance().disableKeyCheckerForTesting();
        OnboardingPrefManager.getInstance().setOnboardingShown(false);
        mActivityTestRule.startMainActivityOnBlankPage();
    }

    @Test
    @SmallTest
    public void testBravePrivateTabDdgVisible() throws Exception {
        mActivityTestRule.newIncognitoTabFromMenu();
        final IncognitoNewTabPage ntp = (IncognitoNewTabPage) mActivityTestRule.getActivity().getActivityTab()
                .getNativePage();

        TestThreadUtils.runOnUiThreadBlocking(() -> {
            Assert.assertEquals(ntp.getView().findViewById(R.id.ddg_offer_link).getVisibility(), View.VISIBLE);
        });
    }

    @Test
    @SmallTest
    public void testBravePrivateTabDdgGone() throws Exception {
        TestThreadUtils.runOnUiThreadBlocking(() -> {
            TemplateUrl templateUrl = BraveSearchEngineUtils
                    .getTemplateUrlByShortName(BraveDuckDuckGoOfferView.DDG_SEARCH_ENGINE_SHORT_NAME);
            if (templateUrl != null) {
                BraveSearchEngineUtils.setDSEPrefs(templateUrl, true);
                BraveSearchEngineUtils.updateActiveDSE(true);
            }
        });

        mActivityTestRule.newIncognitoTabFromMenu();
        final IncognitoNewTabPage ntp = (IncognitoNewTabPage) mActivityTestRule.getActivity().getActivityTab()
                .getNativePage();

        TestThreadUtils.runOnUiThreadBlocking(() -> {
            Assert.assertEquals(ntp.getView().findViewById(R.id.ddg_offer_link).getVisibility(), View.GONE);
        });
    }
}
