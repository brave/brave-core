/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.autofill;

import android.app.assist.AssistStructure;
import android.app.assist.AssistStructure.ViewNode;
import android.view.View;
import android.view.autofill.AutofillId;
import android.view.inputmethod.EditorInfo;

import androidx.collection.ArrayMap;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;

import java.util.Locale;
import java.util.Map;

/**
 * Traverses an {@link AssistStructure} to find autofillable fields in third-party apps.
 *
 * <p>When Android calls our autofill service, it provides an {@link AssistStructure} — a snapshot
 * of the requesting app's view hierarchy. This class walks that tree to find input fields and
 * classify them by type (name, email, phone, address, etc.).
 *
 * <p>Field detection uses three layers, tried in order (first match wins):
 *
 * <ol>
 *   <li><b>Explicit autofill hints</b> ({@link ViewNode#getAutofillHints}): Standard Android
 *       constants like {@code "postalAddress"} or W3C autocomplete tokens like {@code
 *       "street-address"} set by the app or HTML {@code autocomplete} attribute. Mapped via {@link
 *       #mapAutofillHint}.
 *   <li><b>Heuristic text matching</b> ({@link ViewNode#getHint}, {@link ViewNode#getIdEntry},
 *       {@link ViewNode#getText}): Keyword matching on the field's hint text, resource ID, or
 *       content. E.g. a field with hint "Enter your email" matches "email". Handled by {@link
 *       #inferHint}. Works for native Android apps that don't set autofill hints.
 *   <li><b>HTML attributes</b> ({@link ViewNode#getHtmlInfo}): For WebView nodes, Chromium exposes
 *       HTML metadata including {@code computed-autofill-hints} (Chromium's own field
 *       classification, e.g. {@code "NAME_FULL"}, {@code "ADDRESS_HOME_CITY"}) and standard HTML
 *       {@code name}/{@code label} attributes. This layer handles web forms that don't have {@code
 *       autocomplete} attributes — Chromium still classifies them internally. Handled by {@link
 *       #getHintFromHtmlInfo} and {@link #mapComputedAutofillHint}.
 * </ol>
 *
 * <p>The output is a {@code Map<String, AutofillId>} where keys are canonical hint constants
 * (either standard {@code View.AUTOFILL_HINT_*} or our custom {@code HINT_*} constants) and values
 * are the corresponding field IDs. {@link BraveAutofillServiceImpl} uses these keys to match fields
 * with profile data.
 */
@NullMarked
public class BraveAutofillViewStructureParser {
    // Internal map keys for field types that lack standard Android View.AUTOFILL_HINT_* constants.
    // Android only provides 7 standard hints (name, username, password, emailAddress, phone,
    // postalAddress, postalCode). For everything else (first name, city, state, etc.) we define
    // our own. The values are arbitrary strings — they never leave this service, they just need
    // to be consistent between this parser and BraveAutofillServiceImpl.
    public static final String HINT_FIRST_NAME = "givenName";
    public static final String HINT_MIDDLE_NAME = "additionalName";
    public static final String HINT_LAST_NAME = "familyName";
    public static final String HINT_ADDRESS_LINE2 = "addressLine2";
    public static final String HINT_COMPANY = "company";
    public static final String HINT_CITY = "city";
    public static final String HINT_STATE = "state";
    public static final String HINT_COUNTRY = "country";

    // HTML attribute keys exposed by Chromium on WebView nodes via ViewNode.getHtmlInfo().
    private static final String HTML_ATTR_COMPUTED_AUTOFILL_HINTS = "computed-autofill-hints";
    private static final String HTML_ATTR_NAME = "name";
    private static final String HTML_ATTR_LABEL = "label";

    // Android class name used to identify EditText nodes for Layer 2 text content matching.
    private static final String EDIT_TEXT_CLASS = "EditText";

    // W3C autocomplete tokens (HTML Living Standard §4.10.18.7).
    // Used by Layer 1 to map HTML autocomplete attribute values to canonical hints.
    private static final String W3C_GIVEN_NAME = "given-name";
    private static final String W3C_FAMILY_NAME = "family-name";
    private static final String W3C_ADDITIONAL_NAME = "additional-name";
    private static final String W3C_HONORIFIC_PREFIX = "honorific-prefix";
    private static final String W3C_HONORIFIC_SUFFIX = "honorific-suffix";
    private static final String W3C_CC_NAME = "cc-name";
    private static final String W3C_EMAIL = "email";
    private static final String W3C_TEL = "tel";
    private static final String W3C_TEL_NATIONAL = "tel-national";
    private static final String W3C_TEL_LOCAL = "tel-local";
    private static final String W3C_TEL_COUNTRY_CODE = "tel-country-code";
    private static final String W3C_TEL_AREA_CODE = "tel-area-code";
    private static final String W3C_TEL_EXTENSION = "tel-extension";
    private static final String W3C_STREET_ADDRESS = "street-address";
    private static final String W3C_ADDRESS_LINE1 = "address-line1";
    private static final String W3C_ADDRESS_LINE2 = "address-line2";
    private static final String W3C_ADDRESS_LINE3 = "address-line3";
    private static final String W3C_POSTAL_CODE = "postal-code";
    private static final String W3C_ORGANIZATION = "organization";
    private static final String W3C_ADDRESS_LEVEL1 = "address-level1";
    private static final String W3C_ADDRESS_LEVEL2 = "address-level2";
    private static final String W3C_ADDRESS_LEVEL3 = "address-level3";
    private static final String W3C_COUNTRY = "country";
    private static final String W3C_COUNTRY_NAME = "country-name";

    // Chromium computed-autofill-hints string values. These are the string representations of
    // FieldType enum values, produced by FieldTypeToStringView() in
    // components/autofill/core/browser/field_types.cc using the kTypeNameToFieldType mapping.
    // Used by Layer 3 to map Chromium's internal field classification to canonical hints.
    private static final String COMPUTED_NAME_FULL = "NAME_FULL";
    private static final String COMPUTED_NAME_FIRST = "NAME_FIRST";
    private static final String COMPUTED_NAME_MIDDLE = "NAME_MIDDLE";
    private static final String COMPUTED_NAME_MIDDLE_INITIAL = "NAME_MIDDLE_INITIAL";
    private static final String COMPUTED_NAME_LAST = "NAME_LAST";
    private static final String COMPUTED_NAME_LAST_FIRST = "NAME_LAST_FIRST";
    private static final String COMPUTED_NAME_LAST_SECOND = "NAME_LAST_SECOND";
    private static final String COMPUTED_EMAIL_ADDRESS = "EMAIL_ADDRESS";
    private static final String COMPUTED_PHONE_HOME_WHOLE_NUMBER = "PHONE_HOME_WHOLE_NUMBER";
    private static final String COMPUTED_PHONE_HOME_NUMBER = "PHONE_HOME_NUMBER";
    private static final String COMPUTED_PHONE_HOME_CITY_AND_NUMBER = "PHONE_HOME_CITY_AND_NUMBER";
    private static final String COMPUTED_ADDRESS_HOME_LINE1 = "ADDRESS_HOME_LINE1";
    private static final String COMPUTED_ADDRESS_HOME_STREET_ADDRESS =
            "ADDRESS_HOME_STREET_ADDRESS";
    private static final String COMPUTED_ADDRESS_HOME_STREET_NAME = "ADDRESS_HOME_STREET_NAME";
    private static final String COMPUTED_ADDRESS_HOME_ADDRESS = "ADDRESS_HOME_ADDRESS";
    private static final String COMPUTED_ADDRESS_HOME_LINE2 = "ADDRESS_HOME_LINE2";
    private static final String COMPUTED_ADDRESS_HOME_LINE3 = "ADDRESS_HOME_LINE3";
    private static final String COMPUTED_ADDRESS_HOME_SUBPREMISE = "ADDRESS_HOME_SUBPREMISE";
    private static final String COMPUTED_ADDRESS_HOME_APT = "ADDRESS_HOME_APT";
    private static final String COMPUTED_ADDRESS_HOME_APT_NUM = "ADDRESS_HOME_APT_NUM";
    private static final String COMPUTED_ADDRESS_HOME_CITY = "ADDRESS_HOME_CITY";
    private static final String COMPUTED_ADDRESS_HOME_DEPENDENT_LOCALITY =
            "ADDRESS_HOME_DEPENDENT_LOCALITY";
    private static final String COMPUTED_ADDRESS_HOME_STATE = "ADDRESS_HOME_STATE";
    private static final String COMPUTED_ADDRESS_HOME_ZIP = "ADDRESS_HOME_ZIP";
    private static final String COMPUTED_ADDRESS_HOME_SORTING_CODE = "ADDRESS_HOME_SORTING_CODE";
    private static final String COMPUTED_ADDRESS_HOME_COUNTRY = "ADDRESS_HOME_COUNTRY";
    private static final String COMPUTED_COMPANY_NAME = "COMPANY_NAME";

    // Heuristic keywords for Layer 2 inferHint() matching.
    private static final String KEYWORD_LABEL = "label";
    private static final String KEYWORD_CONTAINER = "container";
    private static final String KEYWORD_PASSWORD = "password";
    private static final String KEYWORD_USERNAME = "username";
    private static final String KEYWORD_LOGIN = "login";
    private static final String KEYWORD_ID = "id";
    private static final String KEYWORD_EMAIL = "email";
    private static final String KEYWORD_NAME = "name";
    private static final String KEYWORD_FIRST = "first";
    private static final String KEYWORD_LAST = "last";
    private static final String KEYWORD_FULL = "full";
    private static final String KEYWORD_PHONE = "phone";
    private static final String KEYWORD_MOBILE = "mobile";
    private static final String KEYWORD_ADDRESS = "address";
    private static final String KEYWORD_APARTMENT = "apartment";
    private static final String KEYWORD_PINCODE = "pincode";
    private static final String KEYWORD_POSTALCODE = "postalcode";
    private static final String KEYWORD_ZIP = "zip";
    private static final String KEYWORD_COMPANY = "company";
    private static final String KEYWORD_ORGANIZATION = "organization";
    private static final String KEYWORD_CITY = "city";
    private static final String KEYWORD_TOWN = "town";
    private static final String KEYWORD_STATE = "state";
    private static final String KEYWORD_REGION = "region";
    private static final String KEYWORD_PREFECTURE = "prefecture";
    private static final String KEYWORD_COUNTRY = "country";

    /**
     * Finds all autofillable fields in the given {@link AssistStructure}.
     *
     * @param structure The assist structure to traverse.
     * @return A map of canonical hint key to {@link AutofillId}.
     */
    public static Map<String, AutofillId> findAutofillableFields(AssistStructure structure) {
        Map<String, AutofillId> fields = new ArrayMap<>();
        int nodes = structure.getWindowNodeCount();
        for (int i = 0; i < nodes; i++) {
            ViewNode node = structure.getWindowNodeAt(i).getRootViewNode();
            addAutofillableFields(fields, node);
        }
        return fields;
    }

    /**
     * Finds all autofillable fields and also collects the corresponding {@link ViewNode}
     * references. The nodes are needed by {@link BraveAutofillServiceImpl} to determine field types
     * (text vs dropdown) when creating {@link android.view.autofill.AutofillValue}s and to read
     * user-entered values on save.
     *
     * @param structure The assist structure to traverse.
     * @param nodesMap Output map of AutofillId to ViewNode.
     * @return A map of canonical hint key to {@link AutofillId}.
     */
    public static Map<String, AutofillId> findAutofillableFields(
            AssistStructure structure, Map<AutofillId, ViewNode> nodesMap) {
        Map<String, AutofillId> fields = new ArrayMap<>();
        int nodes = structure.getWindowNodeCount();
        for (int i = 0; i < nodes; i++) {
            ViewNode node = structure.getWindowNodeAt(i).getRootViewNode();
            addAutofillableFieldsWithNodes(fields, node, nodesMap);
        }
        return fields;
    }

    /**
     * Returns true if the detected fields contain at least one address-related hint. Used to decide
     * whether to offer address autofill (vs skipping login-only forms, etc.).
     */
    public static boolean hasAddressFields(Map<String, AutofillId> fields) {
        return fields.containsKey(View.AUTOFILL_HINT_POSTAL_ADDRESS)
                || fields.containsKey(View.AUTOFILL_HINT_POSTAL_CODE)
                || fields.containsKey(View.AUTOFILL_HINT_NAME)
                || fields.containsKey(HINT_FIRST_NAME)
                || fields.containsKey(HINT_LAST_NAME)
                || fields.containsKey(View.AUTOFILL_HINT_EMAIL_ADDRESS)
                || fields.containsKey(View.AUTOFILL_HINT_PHONE)
                || fields.containsKey(HINT_CITY)
                || fields.containsKey(HINT_STATE)
                || fields.containsKey(HINT_COUNTRY)
                || fields.containsKey(HINT_COMPANY);
    }

    /** Recursively walks the view tree, classifying each node and collecting autofillable ones. */
    private static void addAutofillableFields(Map<String, AutofillId> fields, ViewNode node) {
        String hint = getHint(node, fields);
        if (hint != null) {
            AutofillId id = node.getAutofillId();
            if (!fields.containsKey(hint)) {
                fields.put(hint, id);
            }
        }
        int childrenSize = node.getChildCount();
        for (int i = 0; i < childrenSize; i++) {
            addAutofillableFields(fields, node.getChildAt(i));
        }
    }

    /** Same as {@link #addAutofillableFields} but also records ViewNode references. */
    private static void addAutofillableFieldsWithNodes(
            Map<String, AutofillId> fields, ViewNode node, Map<AutofillId, ViewNode> nodesMap) {
        String hint = getHint(node, fields);
        if (hint != null) {
            AutofillId id = node.getAutofillId();
            if (!fields.containsKey(hint)) {
                fields.put(hint, id);
                nodesMap.put(id, node);
            }
        }
        int childrenSize = node.getChildCount();
        for (int i = 0; i < childrenSize; i++) {
            addAutofillableFieldsWithNodes(fields, node.getChildAt(i), nodesMap);
        }
    }

    /**
     * Classifies a single view node using the three-layer detection strategy.
     *
     * <p>The {@code existingFields} map is checked so that if layer 1 returns a hint that's already
     * taken by another field, we fall through to deeper layers. This handles cases where a form
     * incorrectly uses the same {@code autocomplete} attribute on two fields (e.g. both street
     * address and city have {@code autocomplete="address-line1"}) but Chromium's internal
     * classification (layer 3) correctly distinguishes them.
     *
     * @param existingFields Fields already detected, used to skip duplicate hints.
     * @return A canonical hint key, or null if the node is not a recognized autofill field.
     */
    private static @Nullable String getHint(ViewNode node, Map<String, AutofillId> existingFields) {
        // Layer 1: Explicit autofill hints from the app or HTML autocomplete attribute.
        String[] hints = node.getAutofillHints();
        if (hints != null && hints.length > 0 && hints[0] != null && !hints[0].isEmpty()) {
            String mapped = mapAutofillHint(hints[0]);
            if (mapped != null && !existingFields.containsKey(mapped)) return mapped;
            // Fall through if hint is unrecognized or already taken by another field —
            // deeper layers (especially computed-autofill-hints) may classify better.
        }

        // Layer 2: Heuristic text matching on view hint, resource ID, and EditText content.
        // Skip hints already taken — e.g. id="address-line1" matches "address" but that
        // slot may already be filled by the actual street address field.
        String viewHint = node.getHint();
        String hint = inferHint(node, viewHint);
        if (hint != null && !existingFields.containsKey(hint)) return hint;

        String resourceId = node.getIdEntry();
        hint = inferHint(node, resourceId);
        if (hint != null && !existingFields.containsKey(hint)) return hint;

        CharSequence text = node.getText();
        CharSequence className = node.getClassName();
        if (text != null && className != null && className.toString().contains(EDIT_TEXT_CLASS)) {
            hint = inferHint(node, text.toString());
            if (hint != null && !existingFields.containsKey(hint)) return hint;
        }

        // Layer 3: HTML attributes for WebView nodes (Chromium's computed hints, name, label).
        hint = getHintFromHtmlInfo(node, existingFields);
        if (hint != null && !existingFields.containsKey(hint)) return hint;

        return null;
    }

    /**
     * Layer 3 implementation: reads HTML metadata attributes that Chromium attaches to WebView
     * nodes. Tries in order, skipping hints already taken by other fields:
     *
     * <ol>
     *   <li>{@code computed-autofill-hints}: Chromium's internal field classification.
     *   <li>{@code name}: the HTML {@code name} attribute, run through heuristics.
     *   <li>{@code label}: the associated {@code <label>} text, run through heuristics.
     * </ol>
     *
     * <p>The {@code existingFields} check is important here: if a form has wrong {@code
     * autocomplete} attributes (e.g. city field with {@code autocomplete="address-line1"}),
     * Chromium's computed hint may also reflect the wrong attribute. But the HTML {@code <label>}
     * text (e.g. "City") still identifies the field correctly.
     */
    private static @Nullable String getHintFromHtmlInfo(
            ViewNode node, Map<String, AutofillId> existingFields) {
        if (node.getHtmlInfo() == null || node.getHtmlInfo().getAttributes() == null) {
            return null;
        }

        String computedHints = null;
        String nameAttr = null;
        String labelAttr = null;

        for (android.util.Pair<String, String> attr : node.getHtmlInfo().getAttributes()) {
            if (HTML_ATTR_COMPUTED_AUTOFILL_HINTS.equals(attr.first)) {
                computedHints = attr.second;
            } else if (HTML_ATTR_NAME.equals(attr.first)) {
                nameAttr = attr.second;
            } else if (HTML_ATTR_LABEL.equals(attr.first)) {
                labelAttr = attr.second;
            }
        }

        // Each sub-layer falls through if its result is already taken.
        String hint = mapComputedAutofillHint(computedHints);
        if (hint != null && !existingFields.containsKey(hint)) return hint;

        hint = inferHint(node, nameAttr);
        if (hint != null && !existingFields.containsKey(hint)) return hint;

        hint = inferHint(node, labelAttr);
        if (hint != null && !existingFields.containsKey(hint)) return hint;

        return null;
    }

    /**
     * Maps Chromium's internal field type names (from {@code computed-autofill-hints}) to our
     * canonical hint constants. These are string representations of Chromium's {@code FieldType}
     * enum, produced by {@code FieldTypeToStringView()} in
     * components/autofill/core/browser/field_types.cc. They are the most reliable signal for web
     * forms since Chromium uses ML and heuristics to classify fields.
     */
    private static @Nullable String mapComputedAutofillHint(@Nullable String hint) {
        if (hint == null) return null;
        switch (hint) {
            case COMPUTED_NAME_FULL:
                return View.AUTOFILL_HINT_NAME;
            case COMPUTED_NAME_FIRST:
                return HINT_FIRST_NAME;
            case COMPUTED_NAME_MIDDLE:
            case COMPUTED_NAME_MIDDLE_INITIAL:
                return HINT_MIDDLE_NAME;
            case COMPUTED_NAME_LAST:
            case COMPUTED_NAME_LAST_FIRST:
            case COMPUTED_NAME_LAST_SECOND:
                return HINT_LAST_NAME;
            case COMPUTED_EMAIL_ADDRESS:
                return View.AUTOFILL_HINT_EMAIL_ADDRESS;
            case COMPUTED_PHONE_HOME_WHOLE_NUMBER:
            case COMPUTED_PHONE_HOME_NUMBER:
            case COMPUTED_PHONE_HOME_CITY_AND_NUMBER:
                return View.AUTOFILL_HINT_PHONE;
            case COMPUTED_ADDRESS_HOME_LINE1:
            case COMPUTED_ADDRESS_HOME_STREET_ADDRESS:
            case COMPUTED_ADDRESS_HOME_STREET_NAME:
            case COMPUTED_ADDRESS_HOME_ADDRESS:
                return View.AUTOFILL_HINT_POSTAL_ADDRESS;
            case COMPUTED_ADDRESS_HOME_LINE2:
            case COMPUTED_ADDRESS_HOME_LINE3:
            case COMPUTED_ADDRESS_HOME_SUBPREMISE:
            case COMPUTED_ADDRESS_HOME_APT:
            case COMPUTED_ADDRESS_HOME_APT_NUM:
                return HINT_ADDRESS_LINE2;
            case COMPUTED_ADDRESS_HOME_CITY:
            case COMPUTED_ADDRESS_HOME_DEPENDENT_LOCALITY:
                return HINT_CITY;
            case COMPUTED_ADDRESS_HOME_STATE:
                return HINT_STATE;
            case COMPUTED_ADDRESS_HOME_ZIP:
            case COMPUTED_ADDRESS_HOME_SORTING_CODE:
                return View.AUTOFILL_HINT_POSTAL_CODE;
            case COMPUTED_ADDRESS_HOME_COUNTRY:
                return HINT_COUNTRY;
            case COMPUTED_COMPANY_NAME:
                return HINT_COMPANY;
            default:
                return null;
        }
    }

    /**
     * Layer 2 implementation: keyword-based heuristic matching. Checks if the given string (which
     * may be a view hint, resource ID, text content, or HTML attribute) contains keywords
     * associated with known field types.
     *
     * <p>Skips nodes that look like labels/containers (not actual input fields) and omnibox fields
     * (URL bars should not trigger autofill).
     *
     * <p>As a fallback, returns the raw lowercased hint for any enabled autofillable field that
     * didn't match a known keyword. This ensures we don't silently ignore fields — they'll appear
     * in the detected fields map but won't match any profile data.
     */
    private static @Nullable String inferHint(ViewNode node, @Nullable String actualHint) {
        if (actualHint == null || actualHint.isEmpty()) return null;

        String hint = actualHint.toLowerCase(Locale.getDefault());

        // Skip labels and container views — they're not input fields.
        if (hint.contains(KEYWORD_LABEL) || hint.contains(KEYWORD_CONTAINER)) {
            return null;
        }

        // Skip the browser's URL bar.
        if (isOmnibox(node)) {
            return null;
        }

        if (hint.contains(KEYWORD_PASSWORD)) {
            return View.AUTOFILL_HINT_PASSWORD;
        }
        if (hint.contains(KEYWORD_USERNAME)
                || (hint.contains(KEYWORD_LOGIN) && hint.contains(KEYWORD_ID))) {
            return View.AUTOFILL_HINT_USERNAME;
        }
        if (hint.contains(KEYWORD_EMAIL)) {
            return View.AUTOFILL_HINT_EMAIL_ADDRESS;
        }
        if (hint.equals(KEYWORD_NAME)
                || (hint.contains(KEYWORD_NAME)
                        && (hint.contains(KEYWORD_FIRST)
                                || hint.contains(KEYWORD_LAST)
                                || hint.contains(KEYWORD_FULL)))) {
            return View.AUTOFILL_HINT_NAME;
        }
        if (hint.contains(KEYWORD_PHONE) || hint.contains(KEYWORD_MOBILE)) {
            return View.AUTOFILL_HINT_PHONE;
        }
        if (hint.contains(KEYWORD_ADDRESS) || hint.contains(KEYWORD_APARTMENT)) {
            return View.AUTOFILL_HINT_POSTAL_ADDRESS;
        }
        if (hint.contains(KEYWORD_PINCODE)
                || hint.contains(KEYWORD_POSTALCODE)
                || hint.contains(KEYWORD_ZIP)) {
            return View.AUTOFILL_HINT_POSTAL_CODE;
        }
        if (hint.contains(KEYWORD_COMPANY) || hint.contains(KEYWORD_ORGANIZATION)) {
            return HINT_COMPANY;
        }
        if (hint.contains(KEYWORD_CITY) || hint.contains(KEYWORD_TOWN)) {
            return HINT_CITY;
        }
        if (hint.contains(KEYWORD_STATE)
                || hint.contains(KEYWORD_REGION)
                || hint.contains(KEYWORD_PREFECTURE)) {
            return HINT_STATE;
        }
        if (hint.contains(KEYWORD_COUNTRY)) {
            return HINT_COUNTRY;
        }

        // Fallback: return the raw hint for any enabled autofillable field. These won't match
        // any profile data in BraveAutofillServiceImpl, but they'll be included in the field
        // map so SaveInfo can track them.
        if (node.isEnabled() && node.getAutofillType() != View.AUTOFILL_TYPE_NONE) {
            return hint;
        }
        return null;
    }

    /** Detects the browser's URL bar by checking for URI input type. */
    private static boolean isOmnibox(ViewNode node) {
        int inputType = node.getInputType();
        return (inputType & EditorInfo.TYPE_TEXT_VARIATION_URI)
                == EditorInfo.TYPE_TEXT_VARIATION_URI;
    }

    /**
     * Layer 1 implementation: maps explicit autofill hints to our canonical constants.
     *
     * <p>Handles two categories:
     *
     * <ul>
     *   <li>Standard Android {@code View.AUTOFILL_HINT_*} constants (e.g. {@code "postalAddress"})
     *       — passed through as-is since BraveAutofillServiceImpl uses them directly.
     *   <li>W3C autocomplete tokens (e.g. {@code "street-address"}, {@code "given-name"}) — used by
     *       HTML forms with {@code autocomplete} attributes. Mapped to our canonical constants. See
     *       <a
     *       href="https://html.spec.whatwg.org/multipage/form-control-infrastructure.html#autofilling-form-controls:-the-autocomplete-attribute">
     *       HTML Living Standard §4.10.18.7</a>.
     * </ul>
     */
    private static @Nullable String mapAutofillHint(String hint) {
        switch (hint) {
            // Standard Android View.AUTOFILL_HINT_* constants — pass through.
            case View.AUTOFILL_HINT_NAME:
            case View.AUTOFILL_HINT_USERNAME:
            case View.AUTOFILL_HINT_PASSWORD:
            case View.AUTOFILL_HINT_EMAIL_ADDRESS:
            case View.AUTOFILL_HINT_PHONE:
            case View.AUTOFILL_HINT_POSTAL_ADDRESS:
            case View.AUTOFILL_HINT_POSTAL_CODE:
                return hint;

            // W3C autocomplete tokens.
            // "name" is already handled by View.AUTOFILL_HINT_NAME above (both are "name").
            case W3C_GIVEN_NAME:
                return HINT_FIRST_NAME;
            case W3C_FAMILY_NAME:
                return HINT_LAST_NAME;
            case W3C_ADDITIONAL_NAME:
                return HINT_MIDDLE_NAME;
            case W3C_HONORIFIC_PREFIX:
            case W3C_HONORIFIC_SUFFIX:
                return View.AUTOFILL_HINT_NAME;
            case W3C_CC_NAME:
                return View.AUTOFILL_HINT_NAME;
            case W3C_EMAIL:
                return View.AUTOFILL_HINT_EMAIL_ADDRESS;
            case W3C_TEL:
            case W3C_TEL_NATIONAL:
            case W3C_TEL_LOCAL:
            case W3C_TEL_COUNTRY_CODE:
            case W3C_TEL_AREA_CODE:
            case W3C_TEL_EXTENSION:
                return View.AUTOFILL_HINT_PHONE;
            case W3C_STREET_ADDRESS:
            case W3C_ADDRESS_LINE1:
                return View.AUTOFILL_HINT_POSTAL_ADDRESS;
            case W3C_ADDRESS_LINE2:
            case W3C_ADDRESS_LINE3:
                return HINT_ADDRESS_LINE2;
            case W3C_POSTAL_CODE:
                return View.AUTOFILL_HINT_POSTAL_CODE;
            case W3C_ORGANIZATION:
                return HINT_COMPANY;
            case W3C_ADDRESS_LEVEL2: // city
                return HINT_CITY;
            case W3C_ADDRESS_LEVEL1: // state/region
                return HINT_STATE;
            case W3C_ADDRESS_LEVEL3: // dependent locality
                return HINT_CITY;
            case W3C_COUNTRY:
            case W3C_COUNTRY_NAME:
                return HINT_COUNTRY;

            default:
                return null;
        }
    }
}
