/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import androidx.test.filters.MediumTest;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.Batch;
import org.chromium.base.test.util.CallbackHelper;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Restriction;
import org.chromium.chrome.browser.flags.ChromeSwitches;
import org.chromium.chrome.browser.layouts.LayoutManager;
import org.chromium.chrome.browser.layouts.LayoutStateProvider;
import org.chromium.chrome.browser.layouts.LayoutTestUtils;
import org.chromium.chrome.browser.layouts.LayoutType;
import org.chromium.chrome.browser.omnibox.OmniboxStub;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.search_engines.SearchEngineType;
import org.chromium.chrome.browser.search_engines.TemplateUrlServiceFactory;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.ChromeTabbedActivityTestRule;
import org.chromium.chrome.test.util.NewTabPageTestUtils;
import org.chromium.components.omnibox.AutocompleteInput;
import org.chromium.components.omnibox.OmniboxFocusReason;
import org.chromium.components.search_engines.TemplateUrl;
import org.chromium.components.search_engines.TemplateUrlService;
import org.chromium.ui.base.DeviceFormFactor;

import java.util.concurrent.TimeoutException;
import java.util.concurrent.atomic.AtomicBoolean;

@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
@Batch(Batch.PER_CLASS)
public class BraveToolbarManagerTest {
    @Rule
    public ChromeTabbedActivityTestRule mActivityTestRule = new ChromeTabbedActivityTestRule();

    private BraveToolbarManager mToolbarManager;
    private TemplateUrlService mTemplateUrlService;
    private String mOriginalSearchEngineKeyword;

    @Before
    public void setUp() {
        mActivityTestRule.startMainActivityOnBlankPage();
        mToolbarManager = (BraveToolbarManager) mActivityTestRule.getActivity().getToolbarManager();
    }

    @After
    public void tearDown() {
        if (mToolbarManager != null) {
            ThreadUtils.runOnUiThreadBlocking(mToolbarManager::endFuseboxInput);
        }
        if (mTemplateUrlService != null && mOriginalSearchEngineKeyword != null) {
            ThreadUtils.runOnUiThreadBlocking(
                    () -> mTemplateUrlService.setSearchEngine(mOriginalSearchEngineKeyword));
        }
    }

    @Test
    @MediumTest
    @Restriction(DeviceFormFactor.PHONE)
    public void testAcceleratorInputIsIgnoredWhileGoogleNtpHubHides() throws TimeoutException {
        useGoogleAsDefaultSearchEngine();
        mActivityTestRule.loadUrl("chrome://newtab/");
        NewTabPageTestUtils.waitForNtpLoaded(mActivityTestRule.getActivityTab());

        ThreadUtils.runOnUiThreadBlocking(mToolbarManager::endFuseboxInput);
        assertFalse(
                "Google NTP should not start this test with omnibox input active.",
                ThreadUtils.runOnUiThreadBlocking(mToolbarManager::isUrlBarFocused));

        // Google NTP can request omnibox input before the tab switcher finishes closing.
        // That used to leave both UIs on screen at once.
        LayoutManager layoutManager = mActivityTestRule.getActivity().getLayoutManager();
        LayoutTestUtils.startShowingAndWaitForLayout(layoutManager, LayoutType.HUB, false);

        AtomicBoolean acceleratorInputRequested = new AtomicBoolean();
        AtomicBoolean acceleratorStartedInput = new AtomicBoolean();
        OmniboxStub omniboxStub =
                ThreadUtils.runOnUiThreadBlocking(mToolbarManager::getOmniboxStub);
        assertNotNull("Google NTP did not provide an omnibox stub.", omniboxStub);
        LayoutStateProvider.LayoutStateObserver layoutObserver =
                new LayoutStateProvider.LayoutStateObserver() {
                    @Override
                    public void onStartedHiding(@LayoutType int layoutType) {
                        if (layoutType != LayoutType.HUB) return;

                        acceleratorInputRequested.set(true);

                        // Simulate a tap on the Google search accelerator.
                        mToolbarManager.beginFuseboxInput(
                                new AutocompleteInput(OmniboxFocusReason.ACCELERATOR_TAP));
                        acceleratorStartedInput.set(
                                omniboxStub.getAutocompleteInputForTesting() != null);
                    }
                };

        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    layoutManager.addObserver(layoutObserver);
                    layoutManager.showLayout(LayoutType.BROWSING, true);
                    layoutManager.removeObserver(layoutObserver);
                });

        assertTrue("Hub hide did not request accelerator input.", acceleratorInputRequested.get());
        assertFalse(
                "Accelerator input must not start an omnibox session while Hub is hiding.",
                acceleratorStartedInput.get());
        LayoutTestUtils.waitForLayout(layoutManager, LayoutType.BROWSING);
        assertFalse(
                "Accelerator input must not focus the omnibox before Hub finishes hiding.",
                ThreadUtils.runOnUiThreadBlocking(mToolbarManager::isUrlBarFocused));
    }

    private void useGoogleAsDefaultSearchEngine() throws TimeoutException {
        CallbackHelper templateUrlServiceLoaded = new CallbackHelper();
        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    Profile profile = mActivityTestRule.getActivity().getActivityTab().getProfile();
                    assertFalse("The test must use a regular profile.", profile.isOffTheRecord());

                    mTemplateUrlService = TemplateUrlServiceFactory.getForProfile(profile);
                    mTemplateUrlService.runWhenLoaded(templateUrlServiceLoaded::notifyCalled);
                });
        templateUrlServiceLoaded.waitForCallback("Template URL service did not load.", 0);

        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    TemplateUrl originalSearchEngine =
                            mTemplateUrlService.getDefaultSearchEngineTemplateUrl();
                    assertNotNull(
                            "Regular profile has no default search engine.", originalSearchEngine);
                    mOriginalSearchEngineKeyword = originalSearchEngine.getKeyword();

                    String googleSearchEngineKeyword = "";
                    for (TemplateUrl templateUrl : mTemplateUrlService.getTemplateUrls()) {
                        if (mTemplateUrlService.getSearchEngineTypeFromTemplateUrl(
                                        templateUrl.getKeyword())
                                == SearchEngineType.SEARCH_ENGINE_GOOGLE) {
                            googleSearchEngineKeyword = templateUrl.getKeyword();
                            break;
                        }
                    }
                    assertFalse(
                            "The regular profile has no selectable Google search engine.",
                            googleSearchEngineKeyword.isEmpty());

                    mTemplateUrlService.setSearchEngine(googleSearchEngineKeyword);
                    assertTrue(
                            "The test must run with Google as the normal-profile search engine.",
                            mTemplateUrlService.isDefaultSearchEngineGoogle());
                });
    }
}
