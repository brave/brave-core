/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.quick_search_engines.utils;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.annotation.Config;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.brave.browser.quick_search_engines.settings.QuickSearchEnginesModel;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

/**
 * Unit tests for {@link QuickSearchEnginesSerializer}, covering the current format and the
 * migration from the Gson-written format that shipped in cr152-based and cr153-based builds.
 */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class QuickSearchEnginesSerializerTest {
    /**
     * Captured from a cr152-based install. Per that build's R8 mapping the members are, in order,
     * mShortName, mKeyword, mUrl, mIsEnabled and mType, with mType still an int - hence "e":0.
     */
    private static final String LEGACY_JSON_NUMERIC_TYPE =
            "{\":g\":{\"a\":\"Google\",\"b\":\":g\",\"c\":\"{google:baseURL}search?q\\u003d{searchTerms}\\u0026{google:RLZ}{google:originalQueryForSuggestion}{google:assistedQueryStats}{google:searchFieldtrialParameter}{google:language}{google:prefetchSource}{google:searchClient}{google:sourceId}{google:searchSource}{google:contextualSearchVersion}ie\\u003d{inputEncoding}\",\"d\":true,\"e\":0}," // presubmit: ignore-long-line
                    + "\":yt\":{\"a\":\"YouTube\",\"b\":\":yt\",\"c\":\"https://www.youtube.com/results?search_query\\u003d{searchTerms}\",\"d\":true,\"e\":0},"
                    + "\":b\":{\"a\":\"Bing\",\"b\":\":b\",\"c\":\"https://www.bing.com/search?q\\u003d{searchTerms}\",\"d\":true,\"e\":0},"
                    + "\":e\":{\"a\":\"Ecosia\",\"b\":\":e\",\"c\":\"https://www.ecosia.org/search?tt\\u003d42b8ae98\\u0026q\\u003d{searchTerms}\\u0026addon\\u003dbrave\",\"d\":true,\"e\":0},"
                    + "\":d\":{\"a\":\"DuckDuckGo\",\"b\":\":d\",\"c\":\"https://duckduckgo.com/?q\\u003d{searchTerms}\\u0026t\\u003dbrave\",\"d\":true,\"e\":0},"
                    + "\":q\":{\"a\":\"Qwant\",\"b\":\":q\",\"c\":\"https://www.qwant.com/?q\\u003d{searchTerms}\\u0026client\\u003dbrz-brave\",\"d\":true,\"e\":0},"
                    + "\":sp\":{\"a\":\"Startpage\",\"b\":\":sp\",\"c\":\"https://www.startpage.com/do/search?q\\u003d{searchTerms}\\u0026segment\\u003dstartpage.brave\",\"d\":true,\"e\":0}}";

    /**
     * Captured from a cr153-based install. Same engines and the same member names, but R8 narrowed
     * mType to a boolean there (residualsignature "Z"), so it reads "e":false and each entry
     * carries two booleans instead of one.
     */
    private static final String LEGACY_JSON_BOOLEAN_TYPE =
            LEGACY_JSON_NUMERIC_TYPE.replace("\"e\":0", "\"e\":false");

    private static final String GOOGLE_KEYWORD = ":g";
    private static final String YOUTUBE_KEYWORD = ":yt";
    private static final String STARTPAGE_KEYWORD = ":sp";

    private static final List<String> EXPECTED_KEYWORDS =
            List.of(":g", ":yt", ":b", ":e", ":d", ":q", ":sp");

    @Test
    public void testDeserializeLegacyNumericType() {
        assertLegacyListIsIntact(
                QuickSearchEnginesSerializer.deserialize(LEGACY_JSON_NUMERIC_TYPE));
    }

    @Test
    public void testDeserializeLegacyBooleanType() {
        assertLegacyListIsIntact(
                QuickSearchEnginesSerializer.deserialize(LEGACY_JSON_BOOLEAN_TYPE));
    }

    /** The enabled flag is what users actually customise, so it has to survive the migration. */
    @Test
    public void testDeserializeLegacyKeepsDisabledFlag() {
        // Disable Bing in both encodings: with a numeric type member the entry then has a single
        // false boolean, with a boolean type member it has two.
        String numeric =
                LEGACY_JSON_NUMERIC_TYPE.replace(
                        "\"c\":\"https://www.bing.com/search?q\\u003d{searchTerms}\",\"d\":true",
                        "\"c\":\"https://www.bing.com/search?q\\u003d{searchTerms}\",\"d\":false");
        String bool =
                LEGACY_JSON_BOOLEAN_TYPE.replace(
                        "\"c\":\"https://www.bing.com/search?q\\u003d{searchTerms}\",\"d\":true",
                        "\"c\":\"https://www.bing.com/search?q\\u003d{searchTerms}\",\"d\":false");

        for (String json : List.of(numeric, bool)) {
            Map<String, QuickSearchEnginesModel> parsed =
                    QuickSearchEnginesSerializer.deserialize(json);
            assertNotNull(parsed);
            assertFalse(parsed.get(":b").isEnabled());
            assertTrue(parsed.get(GOOGLE_KEYWORD).isEnabled());
        }
    }

    /**
     * Data written by an unobfuscated build keeps the real field names; shape-matching covers it.
     */
    @Test
    public void testDeserializeLegacyUnobfuscatedNames() {
        String json =
                "{\":g\":{\"mShortName\":\"Google\",\"mKeyword\":\":g\","
                        + "\"mUrl\":\"https://www.google.com/search?q={searchTerms}\","
                        + "\"mIsEnabled\":true,\"mType\":0}}";
        Map<String, QuickSearchEnginesModel> parsed =
                QuickSearchEnginesSerializer.deserialize(json);
        assertNotNull(parsed);
        QuickSearchEnginesModel google = parsed.get(GOOGLE_KEYWORD);
        assertNotNull(google);
        assertEquals("Google", google.getShortName());
        assertEquals("https://www.google.com/search?q={searchTerms}", google.getUrl());
        assertTrue(google.isEnabled());
    }

    /** A legacy list survives a migrate-then-save-then-load round trip unchanged. */
    @Test
    public void testLegacyRoundTripsThroughCurrentFormat() {
        Map<String, QuickSearchEnginesModel> migrated =
                QuickSearchEnginesSerializer.deserialize(LEGACY_JSON_BOOLEAN_TYPE);
        assertNotNull(migrated);

        String current = QuickSearchEnginesSerializer.serialize(migrated);
        assertNotNull(current);
        assertTrue("Current format is a JSON array", current.trim().startsWith("["));

        assertLegacyListIsIntact(QuickSearchEnginesSerializer.deserialize(current));
    }

    @Test
    public void testSerializeDeserializePreservesOrderAndFields() {
        Map<String, QuickSearchEnginesModel> original = new LinkedHashMap<>();
        original.put(
                "z",
                newModel("Zeta", "z", "https://z.example/?q={searchTerms}", /* enabled= */ false));
        original.put(
                "a",
                newModel("Alpha", "a", "https://a.example/?q={searchTerms}", /* enabled= */ true));

        String json = QuickSearchEnginesSerializer.serialize(original);
        assertNotNull(json);
        Map<String, QuickSearchEnginesModel> parsed =
                QuickSearchEnginesSerializer.deserialize(json);
        assertNotNull(parsed);

        assertEquals(List.of("z", "a"), new ArrayList<>(parsed.keySet()));
        assertEquals("Zeta", parsed.get("z").getShortName());
        assertFalse(parsed.get("z").isEnabled());
        assertTrue(parsed.get("a").isEnabled());
        assertEquals(
                QuickSearchEnginesModel.QuickSearchEnginesModelType.SEARCH_ENGINE,
                parsed.get("a").getType());
    }

    @Test
    public void testDeserializeRejectsUnusableValues() {
        assertNull(QuickSearchEnginesSerializer.deserialize(""));
        assertNull(QuickSearchEnginesSerializer.deserialize("not json"));
        assertNull(QuickSearchEnginesSerializer.deserialize("[]"));
        assertNull(QuickSearchEnginesSerializer.deserialize("{}"));
        // Well formed JSON, but nothing that looks like a search engine entry.
        assertNull(QuickSearchEnginesSerializer.deserialize("{\":g\":{\"a\":\"Google\"}}"));
    }

    /** A single unreadable entry is dropped; the rest of the list still migrates. */
    @Test
    public void testDeserializeLegacyDropsOnlyTheBadEntry() {
        String json =
                LEGACY_JSON_NUMERIC_TYPE.replace(
                        "\":b\":{\"a\":\"Bing\",\"b\":\":b\",\"c\":\"https://www.bing.com/search?q\\u003d{searchTerms}\",\"d\":true,\"e\":0}",
                        "\":b\":{\"a\":\"Bing\",\"b\":\":b\",\"d\":true,\"e\":0}");
        Map<String, QuickSearchEnginesModel> parsed =
                QuickSearchEnginesSerializer.deserialize(json);
        assertNotNull(parsed);
        assertNull(parsed.get(":b"));
        assertEquals(EXPECTED_KEYWORDS.size() - 1, parsed.size());
        assertNotNull(parsed.get(GOOGLE_KEYWORD));
        assertNotNull(parsed.get(STARTPAGE_KEYWORD));
    }

    private static void assertLegacyListIsIntact(Map<String, QuickSearchEnginesModel> parsed) {
        assertNotNull(parsed);
        assertEquals(EXPECTED_KEYWORDS, new ArrayList<>(parsed.keySet()));

        QuickSearchEnginesModel google = parsed.get(GOOGLE_KEYWORD);
        assertNotNull(google);
        assertEquals("Google", google.getShortName());
        assertEquals(GOOGLE_KEYWORD, google.getKeyword());
        assertTrue(google.getUrl().startsWith("{google:baseURL}search?q={searchTerms}"));
        assertTrue(google.isEnabled());
        assertEquals(
                QuickSearchEnginesModel.QuickSearchEnginesModelType.SEARCH_ENGINE,
                google.getType());

        QuickSearchEnginesModel youtube = parsed.get(YOUTUBE_KEYWORD);
        assertNotNull(youtube);
        assertEquals("YouTube", youtube.getShortName());
        assertEquals(
                "https://www.youtube.com/results?search_query={searchTerms}", youtube.getUrl());

        // Gson escaped '=' and '&'; parsing must give the literal characters back.
        QuickSearchEnginesModel startpage = parsed.get(STARTPAGE_KEYWORD);
        assertNotNull(startpage);
        assertEquals(
                "https://www.startpage.com/do/search?q={searchTerms}&segment=startpage.brave",
                startpage.getUrl());
    }

    private static QuickSearchEnginesModel newModel(
            String shortName, String keyword, String url, boolean enabled) {
        return new QuickSearchEnginesModel(
                shortName,
                keyword,
                url,
                enabled,
                QuickSearchEnginesModel.QuickSearchEnginesModelType.SEARCH_ENGINE);
    }
}
