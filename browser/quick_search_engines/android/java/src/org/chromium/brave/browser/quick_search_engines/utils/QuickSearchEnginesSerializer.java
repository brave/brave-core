/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.quick_search_engines.utils;

import androidx.annotation.VisibleForTesting;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import org.chromium.base.Log;
import org.chromium.brave.browser.quick_search_engines.settings.QuickSearchEnginesModel;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;

import java.util.Iterator;
import java.util.LinkedHashMap;
import java.util.Map;

/**
 * Reads and writes the quick search engines list as JSON, with the member names spelled out
 * explicitly.
 *
 * <p>This replaces a Gson round-trip that reflected over {@link QuickSearchEnginesModel}'s fields,
 * which tied the stored format to whatever R8 emitted for the build doing the writing. That broke
 * in two separate ways, both seen in the field:
 *
 * <ul>
 *   <li>Member types. The cr152-based and cr153-based mappings agree on the names ("a".."e", in
 *       declaration order), but the cr153-based build narrowed {@code int mType} to a boolean - R8
 *       records this as {@code residualsignature: "Z"} - so it stored false where the cr152-based
 *       build had stored 0. Reading the older list then threw "Expected a boolean but was NUMBER".
 *   <li>Member names. An obfuscated build and an unobfuscated one never agree on them, so a list
 *       written by a release build matched nothing when read by a local debug build. Gson left
 *       every member at its default and handed back entries with a null keyword, which blew up
 *       further along as "NullPointerException: key == null" while re-keying the list.
 * </ul>
 *
 * Either way the browser crashed on the omnibox and Settings paths. See
 * https://github.com/brave/brave-browser/issues/58634.
 *
 * <p>Spelling the names out here decouples the stored format from both: R8 can neither rename these
 * members nor change the type they are written as. The legacy reader below goes further and relies
 * on neither, since it has to cope with whatever any past build produced.
 *
 * <p>Two formats are understood:
 *
 * <ul>
 *   <li>Current: a JSON array of objects with the member names below. Array order is the list
 *       order.
 *   <li>Legacy: the Gson output, a JSON object keyed by search engine keyword whose values have
 *       R8-mangled member names. Read only, so existing lists survive the upgrade.
 * </ul>
 *
 * The two are told apart by their outermost type, so no version field is needed.
 */
@NullMarked
public final class QuickSearchEnginesSerializer {
    private static final String TAG = "QSESerializer";

    private static final String KEY_KEYWORD = "keyword";
    private static final String KEY_SHORT_NAME = "shortName";
    private static final String KEY_URL = "url";
    private static final String KEY_IS_ENABLED = "isEnabled";
    private static final String KEY_TYPE = "type";

    // Every search engine template URL contains this placeholder, which is what lets the legacy
    // reader tell the URL apart from the display name without relying on member names.
    private static final String SEARCH_TERMS_PLACEHOLDER = "{searchTerms}";

    private QuickSearchEnginesSerializer() {}

    /** Serializes {@code searchEnginesMap} in the current format. Returns null on failure. */
    public static @Nullable String serialize(
            Map<String, QuickSearchEnginesModel> searchEnginesMap) {
        JSONArray array = new JSONArray();
        try {
            for (Map.Entry<String, QuickSearchEnginesModel> entry : searchEnginesMap.entrySet()) {
                QuickSearchEnginesModel model = entry.getValue();
                if (model == null) continue;
                JSONObject object = new JSONObject();
                // Prefer the map key: it is what every caller looks entries up by.
                object.put(KEY_KEYWORD, entry.getKey());
                object.put(KEY_SHORT_NAME, model.getShortName());
                object.put(KEY_URL, model.getUrl());
                object.put(KEY_IS_ENABLED, model.isEnabled());
                object.put(KEY_TYPE, model.getType());
                array.put(object);
            }
        } catch (JSONException e) {
            Log.e(TAG, "Could not serialize quick search engines", e);
            return null;
        }
        return array.toString();
    }

    /**
     * Parses either supported format. Returns null when {@code json} is empty, unparsable, or
     * carries nothing usable, in which case the caller should fall back to its defaults.
     */
    public static @Nullable Map<String, QuickSearchEnginesModel> deserialize(String json) {
        if (json.isEmpty()) return null;
        String trimmed = json.trim();
        if (trimmed.startsWith("[")) return deserializeCurrent(trimmed);
        if (trimmed.startsWith("{")) return deserializeLegacy(trimmed);
        Log.e(TAG, "Unrecognized quick search engines value");
        return null;
    }

    @VisibleForTesting
    static @Nullable Map<String, QuickSearchEnginesModel> deserializeCurrent(String json) {
        Map<String, QuickSearchEnginesModel> searchEnginesMap = new LinkedHashMap<>();
        try {
            JSONArray array = new JSONArray(json);
            for (int i = 0; i < array.length(); i++) {
                JSONObject object = array.optJSONObject(i);
                if (object == null) continue;
                String keyword = object.optString(KEY_KEYWORD, "");
                if (keyword.isEmpty()) continue;
                searchEnginesMap.put(
                        keyword,
                        new QuickSearchEnginesModel(
                                object.optString(KEY_SHORT_NAME, ""),
                                keyword,
                                object.optString(KEY_URL, ""),
                                object.optBoolean(KEY_IS_ENABLED, true),
                                object.optInt(
                                        KEY_TYPE,
                                        QuickSearchEnginesModel.QuickSearchEnginesModelType
                                                .SEARCH_ENGINE)));
            }
        } catch (JSONException e) {
            Log.e(TAG, "Could not parse quick search engines", e);
            return null;
        }
        return searchEnginesMap.isEmpty() ? null : searchEnginesMap;
    }

    /**
     * Reads the Gson-written format, where each entry's member names are whatever R8 assigned in
     * the build that wrote it (typically "a".."e", or the unobfuscated field names for a local
     * debug build).
     *
     * <p>The names are unusable, so members are identified by type instead:
     *
     * <ul>
     *   <li>keyword: the object key. The entry repeats it as a string, which is skipped.
     *   <li>url: of the remaining strings, the one holding {@code {searchTerms}} - every search
     *       engine template URL has it.
     *   <li>shortName: the string that is left.
     *   <li>isEnabled: any boolean member that is true. Builds differ in how they stored the type
     *       member - cr152-based wrote the number 0, cr153-based the boolean false, after R8
     *       narrowed the field - so an entry may carry one boolean or two. Type is always
     *       SEARCH_ENGINE for a stored entry (AI_ASSISTANT rows are built for the view and never
     *       saved), so it is always 0 or false either way, and a true boolean can only be the
     *       enabled flag. Deciding this by value rather than by position also keeps it independent
     *       of {@link JSONObject#keys()} iteration order.
     *   <li>type: not read. Always SEARCH_ENGINE, per above.
     * </ul>
     *
     * <p>An entry that does not fit is dropped - the caller re-adds it from {@code
     * TemplateUrlService} with default enablement, losing one flag rather than the whole list.
     */
    @VisibleForTesting
    static @Nullable Map<String, QuickSearchEnginesModel> deserializeLegacy(String json) {
        Map<String, QuickSearchEnginesModel> searchEnginesMap = new LinkedHashMap<>();
        try {
            JSONObject root = new JSONObject(json);
            for (Iterator<String> keys = root.keys(); keys.hasNext(); ) {
                String keyword = keys.next();
                JSONObject entry = root.optJSONObject(keyword);
                if (entry == null) continue;
                QuickSearchEnginesModel model = readLegacyEntry(keyword, entry);
                if (model == null) {
                    Log.w(TAG, "Dropping unrecognized legacy entry");
                    continue;
                }
                searchEnginesMap.put(keyword, model);
            }
        } catch (JSONException e) {
            Log.e(TAG, "Could not parse legacy quick search engines", e);
            return null;
        }
        return searchEnginesMap.isEmpty() ? null : searchEnginesMap;
    }

    private static @Nullable QuickSearchEnginesModel readLegacyEntry(
            String keyword, JSONObject entry) {
        boolean sawBoolean = false;
        boolean isEnabled = false;
        String url = null;
        String shortName = null;
        boolean keywordSeen = false;
        boolean ambiguous = false;

        for (Iterator<String> members = entry.keys(); members.hasNext(); ) {
            Object value = entry.opt(members.next());
            if (value instanceof Boolean booleanValue) {
                sawBoolean = true;
                isEnabled |= booleanValue;
            } else if (value instanceof String stringValue) {
                // Skip the entry's copy of the keyword, once, so that only the display name and
                // the URL are left to tell apart. Skipping just one occurrence keeps an engine
                // whose display name happens to equal its keyword readable.
                if (!keywordSeen && stringValue.equals(keyword)) {
                    keywordSeen = true;
                    continue;
                }
                if (stringValue.contains(SEARCH_TERMS_PLACEHOLDER)) {
                    if (url != null) ambiguous = true;
                    url = stringValue;
                } else {
                    if (shortName != null) ambiguous = true;
                    shortName = stringValue;
                }
            }
            // Numbers are ignored: the only numeric member was the type, which is always
            // SEARCH_ENGINE for a stored entry.
        }

        if (ambiguous || !sawBoolean || url == null || shortName == null) {
            return null;
        }
        return new QuickSearchEnginesModel(
                shortName,
                keyword,
                url,
                isEnabled,
                QuickSearchEnginesModel.QuickSearchEnginesModelType.SEARCH_ENGINE);
    }
}
