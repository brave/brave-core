/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.search_engines;

import androidx.test.filters.SmallTest;

import org.junit.Assert;
import org.junit.Before;
import org.junit.FixMethodOrder;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.MethodSorters;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.base.test.util.Batch;
import org.chromium.base.test.util.CriteriaHelper;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.chrome.test.ChromeBrowserTestRule;
import org.chromium.components.search_engines.BraveTemplateUrlService;
import org.chromium.components.search_engines.TemplateUrl;
import org.chromium.components.search_engines.TemplateUrlService.LoadListener;

import java.util.concurrent.atomic.AtomicBoolean;

/**
 * Tests for Brave's custom search engine functionality on Android. Verifies the
 * BraveTemplateUrlService API works correctly for: - Loading and initializing the service -
 * Managing search engine templates (add/remove/update) - Setting and retrieving the default search
 * engine - Handling search engine preferences
 */
@Batch(Batch.PER_CLASS)
@RunWith(BaseJUnit4ClassRunner.class)
@FixMethodOrder(MethodSorters.NAME_ASCENDING)
public class BraveTemplateUrlServiceTest {
    @Rule public final ChromeBrowserTestRule mChromeBrowserTestRule = new ChromeBrowserTestRule();

    private BraveTemplateUrlService mBraveTemplateUrlService;

    private static final String TEST_TITLE = "test";
    private static final String TEST_KEYWORD = "test";
    private static final String TEST_URL = "https://www.test.com/search?q={searchTerms}";

    @Before
    public void setUp() {
        mBraveTemplateUrlService =
                ThreadUtils.runOnUiThreadBlocking(
                        () ->
                                ((BraveTemplateUrlService)
                                        TemplateUrlServiceFactory.getForProfile(
                                                ProfileManager.getLastUsedRegularProfile())));
    }

    @Test
    @SmallTest
    public void test01_Add() {
        waitForBraveTemplateUrlServiceToLoad();
        addTestSearchEngine();
        verifySearchEngine(TEST_KEYWORD, TEST_TITLE, TEST_KEYWORD, TEST_URL);
    }

    @Test
    @SmallTest
    public void test02_Remove() {
        waitForBraveTemplateUrlServiceToLoad();

        removeSearchEngine(TEST_KEYWORD);
        verifySearchEngineRemoved(TEST_KEYWORD);
    }

    @Test
    @SmallTest
    public void test03_RemoveDefaultSearchEngine() {
        waitForBraveTemplateUrlServiceToLoad();
        addTestSearchEngine();
        verifySearchEngine(TEST_KEYWORD, TEST_TITLE, TEST_KEYWORD, TEST_URL);

        setDefaultSearchEngine(TEST_KEYWORD);
        verifyDefaultSearchEngine(TEST_KEYWORD);

        boolean isRemoved = removeSearchEngine(TEST_KEYWORD);

        // search engine shouldn't removed as we can't remove default search engine.
        Assert.assertEquals(false, isRemoved);
    }

    @Test
    @SmallTest
    public void test04_Update() {
        waitForBraveTemplateUrlServiceToLoad();

        String newTitle = "new title";
        String newKeyword = "new keyword";
        String newUrl = "https://www.new.com/search?q={searchTerms}";

        boolean isUpdated = updateSearchEngine(TEST_KEYWORD, newTitle, newKeyword, newUrl);
        Assert.assertEquals(true, isUpdated);
    }

    /**
     * Adds a test search engine with predefined test values. Runs on UI thread since template URL
     * service operations must be performed there.
     */
    private boolean addTestSearchEngine() {
        return ThreadUtils.runOnUiThreadBlocking(
                () -> mBraveTemplateUrlService.add(TEST_TITLE, TEST_KEYWORD, TEST_URL));
    }

    /**
     * Updates an existing search engine with new values.
     *
     * @param oldKeyword The keyword of the search engine to update
     * @param newTitle The new title to set
     * @param newKeyword The new keyword to set
     * @param newUrl The new URL to set
     */
    private boolean updateSearchEngine(
            String oldKeyword, String newTitle, String newKeyword, String newUrl) {
        return ThreadUtils.runOnUiThreadBlocking(
                () -> mBraveTemplateUrlService.update(oldKeyword, newTitle, newKeyword, newUrl));
    }

    /**
     * Removes a search engine by its keyword.
     *
     * @param keyword The keyword of the search engine to remove
     */
    private boolean removeSearchEngine(String keyword) {
        return ThreadUtils.runOnUiThreadBlocking(() -> mBraveTemplateUrlService.remove(keyword));
    }

    /**
     * Sets a search engine as the default search engine.
     *
     * @param keyword The keyword of the search engine to set as default
     */
    private void setDefaultSearchEngine(String keyword) {
        ThreadUtils.runOnUiThreadBlocking(() -> mBraveTemplateUrlService.setSearchEngine(keyword));
    }

    /**
     * Verifies that a search engine exists with the expected values.
     *
     * @param keyword The keyword to look up the search engine
     * @param expectedTitle The expected title of the search engine
     * @param expectedKeyword The expected keyword of the search engine
     * @param expectedUrl The expected URL of the search engine
     */
    private void verifySearchEngine(
            String keyword, String expectedTitle, String expectedKeyword, String expectedUrl) {
        TemplateUrl templateUrl = getTemplateUrlByKeyword(keyword);
        Assert.assertNotNull(templateUrl);
        Assert.assertEquals(expectedTitle, templateUrl.getShortName());
        Assert.assertEquals(expectedKeyword, templateUrl.getKeyword());
        Assert.assertEquals(expectedUrl, templateUrl.getURL());
    }

    /**
     * Verifies that a search engine was removed.
     *
     * @param keyword The keyword of the search engine that should no longer exist
     */
    private void verifySearchEngineRemoved(String keyword) {
        TemplateUrl templateUrl = getTemplateUrlByKeyword(keyword);
        Assert.assertNull(templateUrl);
    }

    /**
     * Verifies that the default search engine matches the expected one.
     *
     * @param expectedKeyword The keyword of the expected default search engine
     */
    private void verifyDefaultSearchEngine(String expectedKeyword) {
        TemplateUrl defaultSearchEngine =
                ThreadUtils.runOnUiThreadBlocking(
                        () -> mBraveTemplateUrlService.getDefaultSearchEngineTemplateUrl());
        Assert.assertNotNull(defaultSearchEngine);
        Assert.assertEquals(expectedKeyword, defaultSearchEngine.getKeyword());
    }

    /**
     * Retrieves a TemplateUrl object by searching for a matching keyword.
     *
     * @param keyword The keyword to search for
     * @return The TemplateUrl with the matching keyword, or null if not found or if the service is
     *     not initialized
     */
    private TemplateUrl getTemplateUrlByKeyword(String keyword) {
        return ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    if (mBraveTemplateUrlService == null) return null;

                    return mBraveTemplateUrlService.getTemplateUrls().stream()
                            .filter(url -> url.getKeyword().equals(keyword))
                            .findFirst()
                            .orElse(null);
                });
    }

    private void waitForBraveTemplateUrlServiceToLoad() {
        final AtomicBoolean observerNotified = new AtomicBoolean(false);
        final LoadListener listener =
                new LoadListener() {
                    @Override
                    public void onTemplateUrlServiceLoaded() {
                        observerNotified.set(true);
                    }
                };

        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    mBraveTemplateUrlService.registerLoadListener(listener);
                    mBraveTemplateUrlService.load();
                });
        CriteriaHelper.pollInstrumentationThread(
                () -> {
                    return observerNotified.get();
                },
                "Observer wasn't notified of TemplateUrlService load.");
    }
}
