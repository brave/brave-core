/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.custom_search_engines;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import androidx.test.filters.SmallTest;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import org.robolectric.annotation.Config;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.components.search_engines.TemplateUrlService;

/** Unit tests for {@link CustomSearchEnginesManager}. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class CustomSearchEnginesManagerTest {
    private static final String TITLE = "Test Engine";
    private static final String KEYWORD = "test_engine";
    private static final String URL = "https://test.com/search?q=%s";
    private static final String URL_WITH_SEARCH_TERMS = "https://test.com/search?q={searchTerms}";

    @Rule public final MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private TemplateUrlService mTemplateUrlService;

    private CustomSearchEnginesManager mManager;

    @Before
    public void setUp() {
        ChromeSharedPreferences.getInstance()
                .removeKey(BravePreferenceKeys.CUSTOM_SEARCH_ENGINE_KEYWORDS);
        mManager = CustomSearchEnginesManager.getInstance();
    }

    @After
    public void tearDown() {
        ChromeSharedPreferences.getInstance()
                .removeKey(BravePreferenceKeys.CUSTOM_SEARCH_ENGINE_KEYWORDS);
    }

    @Test
    @SmallTest
    public void testAdd_success_returnsTrueAndSavesKeyword() {
        when(mTemplateUrlService.addSearchEngine(TITLE, KEYWORD, URL_WITH_SEARCH_TERMS))
                .thenReturn(true);

        boolean result = mManager.addCustomSearchEngine(mTemplateUrlService, TITLE, KEYWORD, URL);

        assertTrue(result);
        assertTrue(mManager.isCustomSearchEngine(KEYWORD));
    }

    @Test
    @SmallTest
    public void testAdd_failure_returnsFalseAndDoesNotSave() {
        when(mTemplateUrlService.addSearchEngine(TITLE, KEYWORD, URL_WITH_SEARCH_TERMS))
                .thenReturn(false);

        boolean result = mManager.addCustomSearchEngine(mTemplateUrlService, TITLE, KEYWORD, URL);

        assertFalse(result);
        assertFalse(mManager.isCustomSearchEngine(KEYWORD));
    }

    @Test
    @SmallTest
    public void testAdd_convertsPercentSToSearchTermsPlaceholder() {
        when(mTemplateUrlService.addSearchEngine(any(), any(), any())).thenReturn(true);

        mManager.addCustomSearchEngine(mTemplateUrlService, TITLE, KEYWORD, URL);

        verify(mTemplateUrlService).addSearchEngine(TITLE, KEYWORD, URL_WITH_SEARCH_TERMS);
    }

    @Test
    @SmallTest
    public void testIsCustom_returnsFalseWhenPrefsEmpty() {
        assertFalse(mManager.isCustomSearchEngine(KEYWORD));
    }

    @Test
    @SmallTest
    public void testIsCustom_returnsTrueForSavedKeyword() {
        when(mTemplateUrlService.addSearchEngine(any(), any(), any())).thenReturn(true);
        mManager.addCustomSearchEngine(mTemplateUrlService, TITLE, KEYWORD, URL);

        assertTrue(mManager.isCustomSearchEngine(KEYWORD));
    }

    @Test
    @SmallTest
    public void testIsCustom_returnsFalseForUnknownKeyword() {
        when(mTemplateUrlService.addSearchEngine(any(), any(), any())).thenReturn(true);
        mManager.addCustomSearchEngine(mTemplateUrlService, TITLE, KEYWORD, URL);

        assertFalse(mManager.isCustomSearchEngine("other_engine"));
    }

    @Test
    @SmallTest
    public void testRemove_success_returnsTrueAndRemovesKeyword() {
        when(mTemplateUrlService.addSearchEngine(any(), any(), any())).thenReturn(true);
        mManager.addCustomSearchEngine(mTemplateUrlService, TITLE, KEYWORD, URL);

        when(mTemplateUrlService.removeSearchEngine(KEYWORD)).thenReturn(true);

        boolean result = mManager.removeCustomSearchEngine(mTemplateUrlService, KEYWORD);

        assertTrue(result);
        assertFalse(mManager.isCustomSearchEngine(KEYWORD));
    }

    @Test
    @SmallTest
    public void testRemove_failure_returnsFalseAndKeepsKeyword() {
        when(mTemplateUrlService.addSearchEngine(any(), any(), any())).thenReturn(true);
        mManager.addCustomSearchEngine(mTemplateUrlService, TITLE, KEYWORD, URL);

        when(mTemplateUrlService.removeSearchEngine(KEYWORD)).thenReturn(false);

        boolean result = mManager.removeCustomSearchEngine(mTemplateUrlService, KEYWORD);

        assertFalse(result);
        assertTrue(mManager.isCustomSearchEngine(KEYWORD));
    }

    @Test
    @SmallTest
    public void testUpdate_success_replacesOldKeywordWithNew() {
        when(mTemplateUrlService.addSearchEngine(any(), any(), any())).thenReturn(true);
        mManager.addCustomSearchEngine(mTemplateUrlService, TITLE, KEYWORD, URL);

        String newKeyword = "updated_engine";
        String newTitle = "Updated Engine";
        String newUrl = "https://updated.com/search?q=%s";
        String newUrlWithSearchTerms = "https://updated.com/search?q={searchTerms}";
        when(mTemplateUrlService.editSearchEngine(
                        KEYWORD, newTitle, newKeyword, newUrlWithSearchTerms))
                .thenReturn(true);

        boolean result =
                mManager.updateCustomSearchEngine(
                        mTemplateUrlService, KEYWORD, newTitle, newKeyword, newUrl);

        assertTrue(result);
        assertFalse(mManager.isCustomSearchEngine(KEYWORD));
        assertTrue(mManager.isCustomSearchEngine(newKeyword));
    }

    @Test
    @SmallTest
    public void testUpdate_failure_returnsFalseAndKeepsOldKeyword() {
        when(mTemplateUrlService.addSearchEngine(any(), any(), any())).thenReturn(true);
        mManager.addCustomSearchEngine(mTemplateUrlService, TITLE, KEYWORD, URL);

        when(mTemplateUrlService.editSearchEngine(any(), any(), any(), any())).thenReturn(false);

        boolean result =
                mManager.updateCustomSearchEngine(
                        mTemplateUrlService, KEYWORD, "New Title", "new_keyword", URL);

        assertFalse(result);
        assertTrue(mManager.isCustomSearchEngine(KEYWORD));
        assertFalse(mManager.isCustomSearchEngine("new_keyword"));
    }

    @Test
    @SmallTest
    public void testUpdate_convertsPercentSToSearchTermsPlaceholder() {
        when(mTemplateUrlService.addSearchEngine(any(), any(), any())).thenReturn(true);
        mManager.addCustomSearchEngine(mTemplateUrlService, TITLE, KEYWORD, URL);

        String newUrl = "https://updated.com/search?q=%s";
        when(mTemplateUrlService.editSearchEngine(any(), any(), any(), any())).thenReturn(true);

        mManager.updateCustomSearchEngine(mTemplateUrlService, KEYWORD, TITLE, KEYWORD, newUrl);

        verify(mTemplateUrlService)
                .editSearchEngine(
                        KEYWORD, TITLE, KEYWORD, "https://updated.com/search?q={searchTerms}");
    }
}
