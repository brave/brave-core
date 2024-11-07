/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.autofill;

import android.app.assist.AssistStructure;
import android.app.assist.AssistStructure.ViewNode;
import android.content.Context;
import android.service.autofill.Dataset;
import android.service.autofill.FillCallback;
import android.service.autofill.FillContext;
import android.service.autofill.FillRequest;
import android.service.autofill.FillResponse;
import android.service.autofill.SaveCallback;
import android.service.autofill.SaveInfo;
import android.service.autofill.SaveRequest;
import android.support.annotation.Nullable;
import android.view.View;
import android.view.autofill.AutofillId;
import android.view.autofill.AutofillValue;
import android.view.inputmethod.EditorInfo;
import android.widget.RemoteViews;

import androidx.annotation.NonNull;
import androidx.collection.ArrayMap;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.browser.ChromeBackgroundServiceImpl;
import org.chromium.chrome.browser.init.BrowserParts;
import org.chromium.chrome.browser.init.ChromeBrowserInitializer;
import org.chromium.chrome.browser.init.EmptyBrowserParts;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.components.autofill.AutofillProfile;
import org.chromium.components.autofill.FieldType;
import org.chromium.components.autofill.RecordType;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;

public class BraveAutofillBackgroundServiceImpl extends ChromeBackgroundServiceImpl {

    private String mPackageName = ContextUtils.getApplicationContext().getPackageName();
    private FillRequest mFillRequest;
    private FillCallback mFillCallback;
    private SaveRequest mSaveRequest;
    private SaveCallback mSaveCallback;
    private boolean mIsSaveRequest;

    public BraveAutofillBackgroundServiceImpl(
            @NonNull FillRequest request, @NonNull FillCallback callback) {
        mFillRequest = request;
        mFillCallback = callback;
    }

    public BraveAutofillBackgroundServiceImpl(
            @NonNull SaveRequest request, @NonNull SaveCallback callback) {
        mSaveRequest = request;
        mSaveCallback = callback;
        mIsSaveRequest = true;
    }

    @Override
    protected void launchBrowser(Context context, String tag) {
        final BrowserParts parts =
                new EmptyBrowserParts() {
                    @Override
                    public void finishNativeInitialization() {
                        Context context = ContextUtils.getApplicationContext();
                        if (context == null) {
                            if (mIsSaveRequest) {
                                mSaveCallback.onSuccess();
                            } else {
                                mFillCallback.onSuccess(null);
                            }
                            return;
                        }
                        if (mIsSaveRequest) {
                            startsAutoSave(context);
                        } else {
                            startsAutoFill(context);
                        }
                    }

                    @Override
                    public boolean startMinimalBrowser() {
                        return true;
                    }
                };

        ChromeBrowserInitializer.getInstance().handlePreNativeStartupAndLoadLibraries(parts);
        ChromeBrowserInitializer.getInstance().handlePostNativeStartup(false, parts);
    }

    private void startsAutoFill(Context context) {
        List<FillContext> fillContexts = mFillRequest.getFillContexts();
        if (fillContexts.isEmpty()) {
            mFillCallback.onSuccess(null);
            return;
        }

        AssistStructure structure = fillContexts.get(fillContexts.size() - 1).getStructure();
        if (structure == null || structure.getWindowNodeCount() == 0) {
            mFillCallback.onSuccess(null);
            return;
        }

        String packageName = structure.getActivityComponent().getPackageName();
        if (mPackageName.equalsIgnoreCase(packageName)) {
            mFillCallback.onSuccess(null);
            return;
        }
        Map<String, AutofillId> fields = new ArrayMap<>();
        int nodes = structure.getWindowNodeCount();
        for (int i = 0; i < nodes; i++) {
            ViewNode node = structure.getWindowNodeAt(i).getRootViewNode();
            addAutofillableFields(context, fields, node, null);
        }

        if (fields.isEmpty()) {
            mFillCallback.onSuccess(null);
            return;
        }

        if (isAutofillAddress(fields)) {
            fillAddressAutoFill(context, fields);
        } else {
            mFillCallback.onSuccess(null);
        }
    }

    private void addAutofillDataMap(
            Map<String, AutofillId> fields,
            String key,
            String data,
            Map<AutofillId, String> autofillDataMap) {
        if (fields.containsKey(key)) {
            AutofillId id = fields.get(key);
            autofillDataMap.put(id, data);
        }
    }

    private void fillAddressAutoFill(Context context, Map<String, AutofillId> fields) {
        PersonalDataManager personalDataManager =
                PersonalDataManagerFactory.getForProfile(
                        ProfileManager.getLastUsedRegularProfile());
        ArrayList<AutofillProfile> profileList = personalDataManager.getProfilesToSuggest(true);
        FillResponse.Builder fillResponse = new FillResponse.Builder();
        if (profileList != null && !profileList.isEmpty()) {
            for (AutofillProfile profile : profileList) {

                Map<AutofillId, String> autofillDataMap = new HashMap<>();
                addAutofillDataMap(
                        fields, View.AUTOFILL_HINT_NAME, profile.getFullName(), autofillDataMap);
                addAutofillDataMap(
                        fields,
                        View.AUTOFILL_HINT_EMAIL_ADDRESS,
                        profile.getEmailAddress(),
                        autofillDataMap);
                addAutofillDataMap(
                        fields,
                        View.AUTOFILL_HINT_PHONE,
                        profile.getPhoneNumber().replaceAll("\\s", ""),
                        autofillDataMap);
                addAutofillDataMap(
                        fields,
                        View.AUTOFILL_HINT_POSTAL_ADDRESS,
                        profile.getStreetAddress(),
                        autofillDataMap);
                addAutofillDataMap(
                        fields,
                        View.AUTOFILL_HINT_POSTAL_CODE,
                        profile.getPostalCode(),
                        autofillDataMap);
                addAutofillDataMap(
                        fields,
                        context.getResources().getString(R.string.city),
                        profile.getLocality(),
                        autofillDataMap);
                addAutofillDataMap(
                        fields,
                        context.getResources().getString(R.string.state),
                        profile.getRegion(),
                        autofillDataMap);
                addAutofillDataMap(
                        fields,
                        context.getResources().getString(R.string.company),
                        profile.getCompanyName(),
                        autofillDataMap);
                addAutofillDataMap(
                        fields,
                        context.getResources().getString(R.string.country),
                        profile.getCountryCode(),
                        autofillDataMap);
                if (autofillDataMap != null && !autofillDataMap.isEmpty()) {
                    Dataset.Builder dataset = new Dataset.Builder();
                    for (Map.Entry<AutofillId, String> autofillData : autofillDataMap.entrySet()) {
                        String title = autofillData.getValue();
                        String subtitle = profile.getStreetAddress();
                        if (title.equals(subtitle)) {
                            subtitle = profile.getFullName();
                        }
                        RemoteViews presentation =
                                new RemoteViews(mPackageName, R.layout.brave_autofill_service_item);
                        if (title.isEmpty()) {
                            title = subtitle;
                            subtitle = "";
                        }
                        presentation.setTextViewText(R.id.title, title);
                        if (subtitle.isEmpty()) {
                            presentation.setViewVisibility(R.id.subtitle, View.GONE);
                        } else {
                            presentation.setViewVisibility(R.id.subtitle, View.VISIBLE);
                            presentation.setTextViewText(R.id.subtitle, subtitle);
                        }
                        dataset.setValue(
                                autofillData.getKey(), AutofillValue.forText(title), presentation);
                    }

                    fillResponse.addDataset(dataset.build());
                }
            }
        }

        Collection<AutofillId> ids = fields.values();
        AutofillId[] requiredIds = new AutofillId[ids.size()];
        ids.toArray(requiredIds);
        fillResponse.setSaveInfo(
                new SaveInfo.Builder(SaveInfo.SAVE_DATA_TYPE_ADDRESS, requiredIds).build());
        mFillCallback.onSuccess(fillResponse.build());
    }

    private void startsAutoSave(Context context) {
        List<FillContext> fillContexts = mSaveRequest.getFillContexts();
        if (fillContexts.isEmpty()) {
            mSaveCallback.onSuccess();
            return;
        }

        AssistStructure structure = fillContexts.get(fillContexts.size() - 1).getStructure();
        if (structure == null || structure.getWindowNodeCount() == 0) {
            mSaveCallback.onSuccess();
            return;
        }

        Map<String, AutofillId> fields = new ArrayMap<>();
        Map<AutofillId, ViewNode> nodesMap = new ArrayMap<>();
        int nodes = structure.getWindowNodeCount();
        for (int i = 0; i < nodes; i++) {
            ViewNode node = structure.getWindowNodeAt(i).getRootViewNode();
            addAutofillableFields(context, fields, node, nodesMap);
        }

        if (fields.isEmpty()) {
            mSaveCallback.onSuccess();
            return;
        }

        if (isAutofillAddress(fields)) {
            String name = getAutofillValueForSave(fields, nodesMap, View.AUTOFILL_HINT_NAME);
            String email =
                    getAutofillValueForSave(fields, nodesMap, View.AUTOFILL_HINT_EMAIL_ADDRESS);
            String phone = getAutofillValueForSave(fields, nodesMap, View.AUTOFILL_HINT_PHONE);
            String address =
                    getAutofillValueForSave(fields, nodesMap, View.AUTOFILL_HINT_POSTAL_ADDRESS);
            String postalCode =
                    getAutofillValueForSave(fields, nodesMap, View.AUTOFILL_HINT_POSTAL_CODE);
            String city =
                    getAutofillValueForSave(
                            fields, nodesMap, context.getResources().getString(R.string.city));
            String state =
                    getAutofillValueForSave(
                            fields, nodesMap, context.getResources().getString(R.string.state));
            String company =
                    getAutofillValueForSave(
                            fields, nodesMap, context.getResources().getString(R.string.company));
            String country =
                    getAutofillValueForSave(
                            fields, nodesMap, context.getResources().getString(R.string.country));

            AutofillAddress autofillAddress =
                    new AutofillAddress(
                            context,
                            AutofillProfile.builder().build(),
                            PersonalDataManagerFactory.getForProfile(
                                    ProfileManager.getLastUsedRegularProfile()));
            AutofillProfile profile = autofillAddress.getProfile();
            profile.setRecordType(RecordType.ACCOUNT);
            profile.setInfo(FieldType.ADDRESS_HOME_COUNTRY, country);
            profile.setInfo(FieldType.PHONE_HOME_WHOLE_NUMBER, phone);
            profile.setInfo(FieldType.NAME_FULL, name);
            profile.setInfo(FieldType.EMAIL_ADDRESS, email);
            profile.setInfo(FieldType.COMPANY_NAME, company);
            profile.setInfo(FieldType.ADDRESS_HOME_STREET_ADDRESS, address);
            profile.setInfo(FieldType.ADDRESS_HOME_CITY, city);
            profile.setInfo(FieldType.ADDRESS_HOME_ZIP, postalCode);
            profile.setInfo(FieldType.ADDRESS_HOME_STATE, state);

            PersonalDataManager personalDataManager =
                    PersonalDataManagerFactory.getForProfile(
                            ProfileManager.getLastUsedRegularProfile());
            profile.setGUID(personalDataManager.setProfileToLocal(profile));
            autofillAddress.updateAddress(profile);
        }
        mSaveCallback.onSuccess();
    }

    private String getAutofillValueForSave(
            @NonNull Map<String, AutofillId> fields,
            Map<AutofillId, ViewNode> nodesMap,
            String key) {
        String autofillData = "";
        if (fields.containsKey(key)) {
            AutofillId autofillId = fields.get(key);
            ViewNode node = nodesMap.get(autofillId);
            int autofillType = node.getAutofillType();
            AutofillValue autofillValue = node.getAutofillValue();
            if (autofillType == View.AUTOFILL_TYPE_TEXT) {
                autofillData =
                        (autofillValue != null) ? autofillValue.getTextValue().toString() : "";
            }
        }
        return autofillData;
    }

    private void addAutofillableFields(
            Context context,
            @NonNull Map<String, AutofillId> fields,
            @NonNull ViewNode node,
            @Nullable Map<AutofillId, ViewNode> nodesMap) {

        String hint = getHint(context, node);
        if (hint != null) {
            AutofillId id = node.getAutofillId();
            if (!fields.containsKey(hint)) {
                fields.put(hint, id);
                if (nodesMap != null) nodesMap.put(id, node);
            }
        }
        int childrenSize = node.getChildCount();
        for (int i = 0; i < childrenSize; i++) {
            addAutofillableFields(context, fields, node.getChildAt(i), nodesMap);
        }
    }

    @Nullable
    private String getHint(Context context, @NonNull ViewNode node) {
        // First try the explicit autofill hints...

        String[] hints = node.getAutofillHints();
        if (hints != null && hints.length > 0 && hints[0] != null && !hints[0].isEmpty()) {
            return hints[0].toLowerCase(Locale.getDefault());
        }

        // Then try some rudimentary heuristics based on other node properties
        String viewHint = node.getHint();
        String hint = inferHint(context, node, viewHint);

        if (hint != null) {
            return hint;
        }

        String resourceId = node.getIdEntry();
        hint = inferHint(context, node, resourceId);
        if (hint != null) {
            return hint;
        }

        CharSequence text = node.getText();
        CharSequence className = node.getClassName();
        if (text != null && className != null && className.toString().contains("EditText")) {
            hint = inferHint(context, node, text.toString());
            if (hint != null) {
                return hint;
            }
        }
        return null;
    }

    /**
     * Uses heuristics to infer an autofill hint from a {@code string}.
     *
     * @return standard autofill hint, or {@code null} when it could not be inferred.
     */
    @Nullable
    private String inferHint(Context context, ViewNode node, @Nullable String actualHint) {
        if (actualHint == null) return null;

        String hint = actualHint.toLowerCase(Locale.getDefault());
        if (hint.contains(context.getResources().getString(R.string.label))
                || hint.contains(context.getResources().getString(R.string.container))) {
            return null;
        }

        if (isOmnibox(node)) {
            return null;
        }

        if (hint.contains(context.getResources().getString(R.string.password))) {
            return View.AUTOFILL_HINT_PASSWORD;
        }
        if (hint.contains(context.getResources().getString(R.string.username))
                || (hint.contains(context.getResources().getString(R.string.login))
                        && hint.contains(context.getResources().getString(R.string.id)))) {
            return View.AUTOFILL_HINT_USERNAME;
        }
        if (hint.contains(context.getResources().getString(R.string.email))) {
            return View.AUTOFILL_HINT_EMAIL_ADDRESS;
        }
        if (hint.equals(context.getResources().getString(R.string.name))
                || (hint.contains(context.getResources().getString(R.string.name))
                        && (hint.contains(context.getResources().getString(R.string.first))
                                || hint.contains(context.getResources().getString(R.string.last))
                                || hint.contains(
                                        context.getResources().getString(R.string.full))))) {
            return View.AUTOFILL_HINT_NAME;
        }
        if (hint.contains(context.getResources().getString(R.string.phone))
                || hint.contains(context.getResources().getString(R.string.mobile))) {
            return View.AUTOFILL_HINT_PHONE;
        }
        if (hint.contains(context.getResources().getString(R.string.address))
                || hint.contains(context.getResources().getString(R.string.apartment))) {
            return View.AUTOFILL_HINT_POSTAL_ADDRESS;
        }
        if (hint.contains(context.getResources().getString(R.string.pincode))
                || hint.contains(context.getResources().getString(R.string.postalcode))
                || hint.contains(context.getResources().getString(R.string.zip))) {
            return View.AUTOFILL_HINT_POSTAL_CODE;
        }
        if (hint.contains(context.getResources().getString(R.string.company))
                || hint.contains(context.getResources().getString(R.string.organization))) {
            return context.getResources().getString(R.string.company);
        }
        if (hint.contains(context.getResources().getString(R.string.city))
                || hint.contains(context.getResources().getString(R.string.town))) {
            return context.getResources().getString(R.string.city);
        }
        if (hint.contains(context.getResources().getString(R.string.state))
                || hint.contains(context.getResources().getString(R.string.region))
                || hint.contains(context.getResources().getString(R.string.prefecture))) {
            return context.getResources().getString(R.string.state);
        }

        // When everything else fails, return the full string - this is helpful to help app
        // developers visualize when autofill is triggered when it shouldn't (for example, in a
        // chat conversation window), so they can mark the root view of such activities with
        // android:importantForAutofill=noExcludeDescendants
        if (node.isEnabled() && node.getAutofillType() != View.AUTOFILL_TYPE_NONE) {
            return hint;
        }
        return null;
    }

    private boolean isOmnibox(ViewNode node) {

        int inputType = node.getInputType();
        if ((inputType & EditorInfo.TYPE_TEXT_VARIATION_URI)
                == EditorInfo.TYPE_TEXT_VARIATION_URI) {
            return true;
        }
        return false;
    }

    private boolean isAutofillAddress(Map<String, AutofillId> fields) {
        return fields.containsKey(View.AUTOFILL_HINT_POSTAL_ADDRESS)
                || fields.containsKey(View.AUTOFILL_HINT_POSTAL_CODE);
    }
}
