/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.autofill;

import android.app.assist.AssistStructure;
import android.app.assist.AssistStructure.ViewNode;
import android.os.CancellationSignal;
import android.service.autofill.Dataset;
import android.service.autofill.FillCallback;
import android.service.autofill.FillContext;
import android.service.autofill.FillRequest;
import android.service.autofill.FillResponse;
import android.service.autofill.SaveCallback;
import android.service.autofill.SaveInfo;
import android.service.autofill.SaveRequest;
import android.view.View;
import android.view.autofill.AutofillId;
import android.view.autofill.AutofillValue;
import android.widget.RemoteViews;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.base.SplitCompatAutofillService;
import org.chromium.chrome.browser.init.ChromeBrowserInitializer;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.components.autofill.AutofillProfile;
import org.chromium.components.autofill.FieldType;
import org.chromium.components.autofill.RecordType;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Implementation of the Brave autofill service for filling address data in third-party apps.
 *
 * <p>This class lives in the chrome module (split_chrome.apk) and is loaded at runtime by {@link
 * SplitCompatAutofillService} from the base module. It extends {@code
 * SplitCompatAutofillService.Impl} which delegates the Android {@link
 * android.service.autofill.AutofillService} callbacks here.
 *
 * <p>When the user selects Brave as their autofill provider in Android Settings, the system calls
 * {@link #onFillRequest} whenever a fillable field is focused in any third-party app (Chrome,
 * Firefox, native apps, etc.). This class then:
 *
 * <ol>
 *   <li>Initializes the Brave browser engine (needed to access saved profiles).
 *   <li>Parses the app's view hierarchy ({@link AssistStructure}) to detect address fields using
 *       {@link BraveAutofillViewStructureParser}.
 *   <li>Queries Brave's saved address profiles via {@link PersonalDataManager}.
 *   <li>Builds a {@link FillResponse} with {@link Dataset} entries (one per saved profile) that the
 *       system shows as a dropdown popup in the requesting app.
 *   <li>Registers {@link SaveInfo} so the system shows a "Save to Brave?" prompt when the user
 *       manually fills a form and submits it.
 * </ol>
 *
 * <p>The service skips its own package (Brave handles its own autofill internally).
 */
@NullMarked
public class BraveAutofillServiceImpl extends SplitCompatAutofillService.Impl {
    private static final String TAG = "BraveAutofillSvc";

    /**
     * Called by the Android framework when a fillable field is focused in a third-party app.
     *
     * <p>Flow: init browser → parse view structure → detect address fields → query saved profiles →
     * build fill response with datasets and save info → return to framework.
     */
    @Override
    public void onFillRequest(
            FillRequest request, CancellationSignal cancellationSignal, FillCallback callback) {
        if (!initBrowser()) {
            callback.onSuccess(null);
            return;
        }

        List<FillContext> fillContexts = request.getFillContexts();
        if (fillContexts.isEmpty()) {
            callback.onSuccess(null);
            return;
        }

        // Get the most recent AssistStructure (the last one is the current screen).
        AssistStructure structure = fillContexts.get(fillContexts.size() - 1).getStructure();
        if (structure == null || structure.getWindowNodeCount() == 0) {
            callback.onSuccess(null);
            return;
        }

        // Skip our own app — Brave handles its own autofill internally.
        String packageName = structure.getActivityComponent().getPackageName();
        if (ContextUtils.getApplicationContext().getPackageName().equalsIgnoreCase(packageName)) {
            callback.onSuccess(null);
            return;
        }

        // Parse the view hierarchy to find autofillable fields and keep a reference to each
        // ViewNode (needed later to determine field types like text vs dropdown).
        Map<AutofillId, ViewNode> nodesMap = new HashMap<>();
        Map<String, AutofillId> fields =
                BraveAutofillViewStructureParser.findAutofillableFields(structure, nodesMap);
        if (fields.isEmpty()) {
            callback.onSuccess(null);
            return;
        }

        if (BraveAutofillViewStructureParser.hasAddressFields(fields)) {
            fillAddressAutofill(fields, nodesMap, callback);
        } else {
            callback.onSuccess(null);
        }
    }

    /**
     * Called by the Android framework after the user fills a form manually and the system shows a
     * "Save to Brave?" prompt (triggered by the {@link SaveInfo} we set in {@link
     * #fillAddressAutofill}). Only called if the user taps "Save".
     *
     * <p>Extracts the filled values from the view structure and saves them as a new address profile
     * in Brave's {@link PersonalDataManager}.
     */
    @Override
    public void onSaveRequest(SaveRequest request, SaveCallback callback) {
        if (!initBrowser()) {
            callback.onSuccess();
            return;
        }

        List<FillContext> fillContexts = request.getFillContexts();
        if (fillContexts.isEmpty()) {
            callback.onSuccess();
            return;
        }

        AssistStructure structure = fillContexts.get(fillContexts.size() - 1).getStructure();
        if (structure == null || structure.getWindowNodeCount() == 0) {
            callback.onSuccess();
            return;
        }

        Map<AutofillId, ViewNode> nodesMap = new HashMap<>();
        Map<String, AutofillId> fields =
                BraveAutofillViewStructureParser.findAutofillableFields(structure, nodesMap);
        if (fields.isEmpty()) {
            callback.onSuccess();
            return;
        }

        if (BraveAutofillViewStructureParser.hasAddressFields(fields)) {
            saveAddressAutofill(fields, nodesMap);
        }

        callback.onSuccess();
    }

    /**
     * Initializes the Brave browser engine. The browser must be running to access saved profiles
     * via {@link PersonalDataManager}. Returns false if initialization fails.
     */
    private boolean initBrowser() {
        try {
            ChromeBrowserInitializer.getInstance().handleSynchronousStartup();
        } catch (Exception e) {
            Log.e(TAG, "Failed to initialize browser", e);
            return false;
        }
        return true;
    }

    /**
     * Builds a {@link FillResponse} containing one {@link Dataset} per saved address profile.
     *
     * <p>For each profile, maps detected form fields to profile data (name, email, phone, address,
     * etc.). Each field in the dataset gets an appropriate {@link AutofillValue}: {@code forText()}
     * for text inputs or {@code forList(index)} for dropdowns/selects.
     *
     * <p>The response also includes {@link SaveInfo} so the system prompts "Save to Brave?" when
     * the user fills a form manually — even if we had no profiles to suggest.
     *
     * @param fields Map of hint key → AutofillId from the view structure parser.
     * @param nodesMap Map of AutofillId → ViewNode for determining field types.
     * @param callback The framework callback to deliver the response.
     */
    private void fillAddressAutofill(
            Map<String, AutofillId> fields,
            Map<AutofillId, ViewNode> nodesMap,
            FillCallback callback) {
        PersonalDataManager personalDataManager =
                PersonalDataManagerFactory.getForProfile(
                        // This is a top-level entry point (AutofillService) with no
                        // Profile passed in from the framework.
                        ProfileManager.getLastUsedRegularProfile());
        ArrayList<AutofillProfile> profileList = personalDataManager.getProfilesToSuggest();

        FillResponse.Builder fillResponse = new FillResponse.Builder();
        String appPackageName = ContextUtils.getApplicationContext().getPackageName();

        if (profileList == null) {
            profileList = new ArrayList<>();
        }

        // Build one Dataset per saved profile. Each dataset fills all matching fields at once
        // when the user taps the suggestion.
        for (AutofillProfile profile : profileList) {
            // Map detected field hints to profile data values. Only fields that exist in
            // both the form and the profile (non-empty) get included.
            Map<AutofillId, String> autofillDataMap = new HashMap<>();
            String fullName = profile.getInfo(FieldType.NAME_FULL);
            String firstName = profile.getInfo(FieldType.NAME_FIRST);
            String lastName = profile.getInfo(FieldType.NAME_LAST);
            // If individual name parts are empty, try to derive them from the full name
            // so forms with separate given-name/family-name fields still work.
            if (!isNullOrEmpty(fullName)) {
                String[] parts = fullName.trim().split("\\s+", 2);
                if (isNullOrEmpty(firstName)) {
                    firstName = parts[0];
                }
                if (isNullOrEmpty(lastName) && parts.length > 1) {
                    lastName = parts[1];
                }
            }
            mapField(fields, View.AUTOFILL_HINT_NAME, fullName, autofillDataMap);
            mapField(
                    fields,
                    BraveAutofillViewStructureParser.HINT_FIRST_NAME,
                    firstName,
                    autofillDataMap);
            mapField(
                    fields,
                    BraveAutofillViewStructureParser.HINT_MIDDLE_NAME,
                    profile.getInfo(FieldType.NAME_MIDDLE),
                    autofillDataMap);
            mapField(
                    fields,
                    BraveAutofillViewStructureParser.HINT_LAST_NAME,
                    lastName,
                    autofillDataMap);
            mapField(
                    fields,
                    View.AUTOFILL_HINT_EMAIL_ADDRESS,
                    profile.getInfo(FieldType.EMAIL_ADDRESS),
                    autofillDataMap);
            // Strip whitespace from phone numbers — profiles may store them as
            // "+1 234 567 8901" but phone input fields typically expect no spaces.
            String phone = profile.getInfo(FieldType.PHONE_HOME_WHOLE_NUMBER);
            if (phone != null) {
                phone = phone.replaceAll("\\s", "");
            }
            mapField(fields, View.AUTOFILL_HINT_PHONE, phone, autofillDataMap);
            mapField(
                    fields,
                    View.AUTOFILL_HINT_POSTAL_ADDRESS,
                    profile.getInfo(FieldType.ADDRESS_HOME_STREET_ADDRESS),
                    autofillDataMap);
            mapField(
                    fields,
                    BraveAutofillViewStructureParser.HINT_ADDRESS_LINE2,
                    profile.getInfo(FieldType.ADDRESS_HOME_LINE2),
                    autofillDataMap);
            mapField(
                    fields,
                    View.AUTOFILL_HINT_POSTAL_CODE,
                    profile.getInfo(FieldType.ADDRESS_HOME_ZIP),
                    autofillDataMap);
            mapField(
                    fields,
                    BraveAutofillViewStructureParser.HINT_CITY,
                    profile.getInfo(FieldType.ADDRESS_HOME_CITY),
                    autofillDataMap);
            mapField(
                    fields,
                    BraveAutofillViewStructureParser.HINT_STATE,
                    profile.getInfo(FieldType.ADDRESS_HOME_STATE),
                    autofillDataMap);
            mapField(
                    fields,
                    BraveAutofillViewStructureParser.HINT_COMPANY,
                    profile.getInfo(FieldType.COMPANY_NAME),
                    autofillDataMap);
            // ADDRESS_HOME_COUNTRY returns a country code (e.g. "US"). Convert it to
            // a display name (e.g. "United States") so it matches dropdown options.
            String countryCode = profile.getInfo(FieldType.ADDRESS_HOME_COUNTRY);
            String countryValue = countryCode;
            if (countryCode != null && countryCode.length() == 2) {
                String displayName = new java.util.Locale("", countryCode).getDisplayCountry();
                if (!displayName.equals(countryCode)) {
                    countryValue = displayName;
                }
            }
            mapField(
                    fields,
                    BraveAutofillViewStructureParser.HINT_COUNTRY,
                    countryValue,
                    autofillDataMap);

            if (!autofillDataMap.isEmpty()) {
                Dataset.Builder dataset = new Dataset.Builder();
                for (Map.Entry<AutofillId, String> entry : autofillDataMap.entrySet()) {
                    AutofillId fieldId = entry.getKey();
                    String value = entry.getValue();
                    ViewNode node = nodesMap.get(fieldId);

                    // Create the right AutofillValue type: forText() for text inputs,
                    // forList(index) for <select> dropdowns.
                    AutofillValue autofillValue = createAutofillValue(node, value);
                    if (autofillValue == null) {
                        continue;
                    }

                    // Build the popup presentation. Title is the field value, subtitle is
                    // the address (or name if the field value is already the address).
                    String title = value;
                    String subtitleOrNull = profile.getInfo(FieldType.ADDRESS_HOME_STREET_ADDRESS);
                    String subtitle = subtitleOrNull != null ? subtitleOrNull : "";
                    if (title.equals(subtitle)) {
                        String nameOrNull = profile.getInfo(FieldType.NAME_FULL);
                        subtitle = nameOrNull != null ? nameOrNull : "";
                    }
                    if (title.isEmpty()) {
                        title = subtitle;
                        subtitle = "";
                    }

                    RemoteViews presentation =
                            new RemoteViews(appPackageName, R.layout.brave_autofill_service_item);
                    presentation.setTextViewText(R.id.title, title);
                    if (subtitle.isEmpty()) {
                        presentation.setViewVisibility(R.id.subtitle, View.GONE);
                    } else {
                        presentation.setViewVisibility(R.id.subtitle, View.VISIBLE);
                        presentation.setTextViewText(R.id.subtitle, subtitle);
                    }
                    dataset.setValue(fieldId, autofillValue, presentation);
                }
                fillResponse.addDataset(dataset.build());
            }
        }

        try {
            // Set up SaveInfo so Android shows "Save to Brave?" after form submission.
            // We require just one key field (name, email, etc.) to be filled — the rest are
            // optional. This way the save prompt appears even if the user skips some fields.
            AutofillId requiredId = pickRequiredSaveField(fields);
            SaveInfo.Builder saveInfoBuilder =
                    new SaveInfo.Builder(
                            SaveInfo.SAVE_DATA_TYPE_ADDRESS, new AutofillId[] {requiredId});
            List<AutofillId> optionalIds = new ArrayList<>();
            for (AutofillId id : fields.values()) {
                if (!id.equals(requiredId)) {
                    optionalIds.add(id);
                }
            }
            if (!optionalIds.isEmpty()) {
                saveInfoBuilder.setOptionalIds(optionalIds.toArray(new AutofillId[0]));
            }
            fillResponse.setSaveInfo(saveInfoBuilder.build());
            FillResponse response = fillResponse.build();
            callback.onSuccess(response);
        } catch (Exception e) {
            Log.e(TAG, "Error sending FillResponse", e);
            callback.onSuccess(null);
        }
    }

    /**
     * Extracts user-entered values from the filled form and saves them as a new address profile.
     * Called from {@link #onSaveRequest} after the user confirms the system's "Save to Brave?"
     * prompt.
     *
     * <p>If the form has a single full-name field, stores it as {@code NAME_FULL}. If the form has
     * separate first/middle/last fields, stores them individually so they round-trip correctly when
     * filling forms with either layout.
     */
    private void saveAddressAutofill(
            Map<String, AutofillId> fields, Map<AutofillId, ViewNode> nodesMap) {
        String name = getFieldValue(fields, nodesMap, View.AUTOFILL_HINT_NAME);
        String firstName =
                getFieldValue(fields, nodesMap, BraveAutofillViewStructureParser.HINT_FIRST_NAME);
        String middleName =
                getFieldValue(fields, nodesMap, BraveAutofillViewStructureParser.HINT_MIDDLE_NAME);
        String lastName =
                getFieldValue(fields, nodesMap, BraveAutofillViewStructureParser.HINT_LAST_NAME);
        String email = getFieldValue(fields, nodesMap, View.AUTOFILL_HINT_EMAIL_ADDRESS);
        String phone = getFieldValue(fields, nodesMap, View.AUTOFILL_HINT_PHONE);
        String address = getFieldValue(fields, nodesMap, View.AUTOFILL_HINT_POSTAL_ADDRESS);
        String addressLine2 =
                getFieldValue(
                        fields, nodesMap, BraveAutofillViewStructureParser.HINT_ADDRESS_LINE2);
        String postalCode = getFieldValue(fields, nodesMap, View.AUTOFILL_HINT_POSTAL_CODE);
        String city = getFieldValue(fields, nodesMap, BraveAutofillViewStructureParser.HINT_CITY);
        String state = getFieldValue(fields, nodesMap, BraveAutofillViewStructureParser.HINT_STATE);
        String company =
                getFieldValue(fields, nodesMap, BraveAutofillViewStructureParser.HINT_COMPANY);
        String country =
                getFieldValue(fields, nodesMap, BraveAutofillViewStructureParser.HINT_COUNTRY);

        PersonalDataManager personalDataManager =
                PersonalDataManagerFactory.getForProfile(
                        // This is a top-level entry point (AutofillService) with no
                        // Profile passed in from the framework.
                        ProfileManager.getLastUsedRegularProfile());

        AutofillProfile profile = AutofillProfile.builder().build();
        profile.setRecordType(RecordType.LOCAL_OR_SYNCABLE);
        if (!name.isEmpty()) {
            profile.setInfo(FieldType.NAME_FULL, name);
        } else {
            profile.setInfo(FieldType.NAME_FIRST, firstName);
            profile.setInfo(FieldType.NAME_MIDDLE, middleName);
            profile.setInfo(FieldType.NAME_LAST, lastName);
        }
        profile.setInfo(FieldType.EMAIL_ADDRESS, email);
        profile.setInfo(FieldType.PHONE_HOME_WHOLE_NUMBER, phone);
        profile.setInfo(FieldType.ADDRESS_HOME_STREET_ADDRESS, address);
        profile.setInfo(FieldType.ADDRESS_HOME_LINE2, addressLine2);
        profile.setInfo(FieldType.ADDRESS_HOME_ZIP, postalCode);
        profile.setInfo(FieldType.ADDRESS_HOME_CITY, city);
        profile.setInfo(FieldType.ADDRESS_HOME_STATE, state);
        profile.setInfo(FieldType.COMPANY_NAME, company);
        profile.setInfo(FieldType.ADDRESS_HOME_COUNTRY, country);

        personalDataManager.setProfileToLocal(profile);
    }

    /**
     * Picks the most likely-to-be-filled field as the single required {@link SaveInfo} field.
     * Android only shows the "Save?" prompt if ALL required fields are filled, so we require just
     * one common field and make everything else optional.
     */
    private static AutofillId pickRequiredSaveField(Map<String, AutofillId> fields) {
        String[] preferred = {
            View.AUTOFILL_HINT_NAME,
            BraveAutofillViewStructureParser.HINT_FIRST_NAME,
            View.AUTOFILL_HINT_EMAIL_ADDRESS,
            View.AUTOFILL_HINT_POSTAL_ADDRESS,
            View.AUTOFILL_HINT_PHONE,
        };
        for (String key : preferred) {
            if (fields.containsKey(key)) {
                return fields.get(key);
            }
        }
        return fields.values().iterator().next();
    }

    /**
     * Adds a field to the autofill data map if the form contains that field and the profile has a
     * non-empty value for it. Fields missing from either the form or the profile are silently
     * skipped — the popup only shows fields we can actually fill.
     */
    private static void mapField(
            Map<String, AutofillId> fields,
            String key,
            @Nullable String data,
            Map<AutofillId, String> autofillDataMap) {
        if (data != null && !data.isEmpty() && fields.containsKey(key)) {
            AutofillId id = fields.get(key);
            autofillDataMap.put(id, data);
        }
    }

    private static boolean isNullOrEmpty(@Nullable String s) {
        return s == null || s.isEmpty();
    }

    /**
     * Creates an {@link AutofillValue} appropriate for the given node's type.
     *
     * <p>Text inputs ({@code AUTOFILL_TYPE_TEXT}) get {@code AutofillValue.forText()}.
     * Dropdown/select fields ({@code AUTOFILL_TYPE_LIST}) get {@code AutofillValue.forList(index)}
     * where the index is found by matching the value against the dropdown options. Using the wrong
     * type causes an {@code IllegalStateException} at fill time.
     */
    private static @Nullable AutofillValue createAutofillValue(
            @Nullable ViewNode node, String value) {
        if (node == null) return AutofillValue.forText(value);

        int type = node.getAutofillType();
        if (type == View.AUTOFILL_TYPE_LIST) {
            return createListValue(node, value);
        }
        if (type == View.AUTOFILL_TYPE_TEXT || type == View.AUTOFILL_TYPE_NONE) {
            return AutofillValue.forText(value);
        }
        return null;
    }

    /**
     * Finds the best matching option index for a list/dropdown field ({@code <select>} in HTML).
     *
     * <p>Uses a three-pass strategy to handle the variety of option formats across forms:
     *
     * <ol>
     *   <li>Exact match (case-insensitive): "California" matches "California"
     *   <li>Option starts with value: profile stores "CA", dropdown has "California"
     *   <li>Value starts with option: profile stores "California", dropdown has "CA"
     * </ol>
     *
     * <p>The minimum length of 2 for prefix matching avoids false positives where short strings
     * like state codes accidentally match unrelated options.
     */
    private static @Nullable AutofillValue createListValue(ViewNode node, String value) {
        CharSequence[] options = node.getAutofillOptions();
        if (options == null) return null;

        java.util.Locale locale = java.util.Locale.getDefault();
        String lowerValue = value.toLowerCase(locale);

        // Pass 1: exact match.
        for (int i = 0; i < options.length; i++) {
            if (options[i] != null
                    && options[i].toString().toLowerCase(locale).equals(lowerValue)) {
                return AutofillValue.forList(i);
            }
        }

        // Pass 2: option starts with value (e.g. value "CA" matches option "California").
        if (lowerValue.length() >= 2) {
            for (int i = 0; i < options.length; i++) {
                if (options[i] != null
                        && options[i].toString().toLowerCase(locale).startsWith(lowerValue)) {
                    return AutofillValue.forList(i);
                }
            }
        }

        // Pass 3: value starts with option (e.g. value "California" matches option "CA").
        for (int i = 0; i < options.length; i++) {
            if (options[i] != null) {
                String lowerOption = options[i].toString().toLowerCase(locale);
                if (lowerOption.length() >= 2 && lowerValue.startsWith(lowerOption)) {
                    return AutofillValue.forList(i);
                }
            }
        }

        return null;
    }

    /**
     * Reads the user-entered value from a filled form field. Used by {@link #saveAddressAutofill}
     * to extract what the user typed before saving it as a new profile.
     *
     * <p>Handles both text inputs ({@code AUTOFILL_TYPE_TEXT}) and dropdown/select fields ({@code
     * AUTOFILL_TYPE_LIST}). For dropdowns, reads the selected index and looks up the corresponding
     * option text from the node's autofill options.
     */
    private static String getFieldValue(
            Map<String, AutofillId> fields, Map<AutofillId, ViewNode> nodesMap, String key) {
        if (!fields.containsKey(key)) return "";
        AutofillId autofillId = fields.get(key);
        ViewNode node = nodesMap.get(autofillId);
        if (node == null) return "";
        AutofillValue autofillValue = node.getAutofillValue();
        if (autofillValue == null) return "";

        int type = node.getAutofillType();
        if (type == View.AUTOFILL_TYPE_TEXT) {
            return autofillValue.getTextValue().toString();
        }
        if (type == View.AUTOFILL_TYPE_LIST) {
            int index = autofillValue.getListValue();
            CharSequence[] options = node.getAutofillOptions();
            if (options != null && index >= 0 && index < options.length) {
                return options[index].toString();
            }
        }
        return "";
    }
}
