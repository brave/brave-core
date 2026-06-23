/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.autofill.options;

import static org.chromium.build.NullUtil.assumeNonNull;

import android.content.Context;
import android.os.Bundle;

import androidx.annotation.VisibleForTesting;
import androidx.preference.Preference;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.autofill.settings.HomeOfTransactionsFragment;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.settings.ChromeBaseSettingsFragment;
import org.chromium.chrome.browser.settings.MainSettings;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;
import org.chromium.components.browser_ui.settings.search.PreferenceParser;
import org.chromium.components.browser_ui.settings.search.SettingsIndexData;
import org.chromium.components.user_prefs.UserPrefs;

@NullMarked
public abstract class BraveAutofillOptionsFragmentBase extends ChromeBaseSettingsFragment {
    public static final String PREF_AUTOFILL_PRIVATE_WINDOW = "autofill_private_window";

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        validateAutofillOptionsReferrer(
                getReferrerFromInstanceStateOrLaunchBundle(savedInstanceState, getArguments()));
        addAutofillPrivateWindowPreference();
    }

    @VisibleForTesting(otherwise = VisibleForTesting.PRIVATE)
    static void validateAutofillOptionsReferrer(
            @AutofillOptionsFragment.AutofillOptionsReferrer int referrer) {
        switch (referrer) {
            case AutofillOptionsFragment.AutofillOptionsReferrer.SETTINGS:
            case AutofillOptionsFragment.AutofillOptionsReferrer.PAYMENT_METHODS_FRAGMENT:
            case AutofillOptionsFragment.AutofillOptionsReferrer.AUTOFILL_PROFILES_FRAGMENT:
            case AutofillOptionsFragment.AutofillOptionsReferrer.AUTOFILL_AND_PASSWORDS_FRAGMENT:
            case AutofillOptionsFragment.AutofillOptionsReferrer.AUTOFILL_IDENTITY_DOCS_FRAGMENT:
            case AutofillOptionsFragment.AutofillOptionsReferrer.AUTOFILL_TRAVEL_FRAGMENT:
            case AutofillOptionsFragment.AutofillOptionsReferrer.AUTOFILL_SHOPPING_FRAGMENT:
            case AutofillOptionsFragment.AutofillOptionsReferrer.DEEP_LINK_TO_SETTINGS:
                return;
            case AutofillOptionsFragment.AutofillOptionsReferrer.COUNT:
            default:
                throw new IllegalArgumentException(
                        "Unsupported AutofillOptionsReferrer: " + referrer);
        }
    }

    @VisibleForTesting(otherwise = VisibleForTesting.PRIVATE)
    static @AutofillOptionsFragment.AutofillOptionsReferrer int
            getReferrerFromInstanceStateOrLaunchBundle(
                    @Nullable Bundle savedInstanceState, @Nullable Bundle launchBundle) {
        if (savedInstanceState != null
                && savedInstanceState.containsKey(
                        AutofillOptionsFragment.AUTOFILL_OPTIONS_REFERRER)) {
            return savedInstanceState.getInt(
                    AutofillOptionsFragment.AUTOFILL_OPTIONS_REFERRER,
                    AutofillOptionsFragment.AutofillOptionsReferrer.COUNT);
        }

        Bundle extras = assumeNonNull(launchBundle);
        return extras.getInt(
                AutofillOptionsFragment.AUTOFILL_OPTIONS_REFERRER,
                AutofillOptionsFragment.AutofillOptionsReferrer.COUNT);
    }

    private void addAutofillPrivateWindowPreference() {
        if (findPreference(PREF_AUTOFILL_PRIVATE_WINDOW) != null) {
            return;
        }

        ChromeSwitchPreference autofillPrivateWindowPreference =
                new ChromeSwitchPreference(getPreferenceManager().getContext());
        autofillPrivateWindowPreference.setKey(PREF_AUTOFILL_PRIVATE_WINDOW);
        autofillPrivateWindowPreference.setIcon(R.drawable.ic_autofill);
        autofillPrivateWindowPreference.setTitle(R.string.prefs_autofill_private_window_title);
        autofillPrivateWindowPreference.setSummary(R.string.prefs_autofill_private_window_summary);
        autofillPrivateWindowPreference.setChecked(
                UserPrefs.get(getProfile()).getBoolean(BravePref.BRAVE_AUTOFILL_PRIVATE_WINDOWS));
        autofillPrivateWindowPreference.setOnPreferenceChangeListener(
                (preference, newValue) -> {
                    UserPrefs.get(getProfile())
                            .setBoolean(
                                    BravePref.BRAVE_AUTOFILL_PRIVATE_WINDOWS, (boolean) newValue);
                    return true;
                });
        autofillPrivateWindowPreference.setOrder(3);

        Preference autofillAiCategory =
                findPreference(AutofillOptionsFragment.PREF_AUTOFILL_AI_CATEGORY);
        if (autofillAiCategory != null) {
            autofillAiCategory.setOrder(4);
        }

        getPreferenceScreen().addPreference(autofillPrivateWindowPreference);
        notifyPreferencesUpdated();
    }

    private static Bundle createSettingsArgs() {
        return AutofillOptionsFragment.createRequiredArgs(
                AutofillOptionsFragment.AutofillOptionsReferrer.SETTINGS);
    }

    private static Bundle createHomeOfTransactionsArgs() {
        Bundle args = new Bundle();
        args.putInt(
                HomeOfTransactionsFragment.EXTRA_REFERRER,
                HomeOfTransactionsFragment.AutofillSettingsReferrer.SETTINGS_MENU);
        return args;
    }

    private static void updateAutofillPrivateWindowEntry(
            Context context, SettingsIndexData indexData) {
        SettingsIndexData.Entry autofillPrivateWindowEntry =
                indexData.getEntryForKey(
                        AutofillOptionsFragment.class.getName(), PREF_AUTOFILL_PRIVATE_WINDOW);
        if (autofillPrivateWindowEntry == null) {
            indexData.addEntryForKey(
                    AutofillOptionsFragment.class.getName(),
                    PREF_AUTOFILL_PRIVATE_WINDOW,
                    R.string.prefs_autofill_private_window_title,
                    R.string.prefs_autofill_private_window_summary,
                    createSettingsArgs());
            return;
        }

        indexData.updateEntry(
                autofillPrivateWindowEntry.id,
                new SettingsIndexData.Entry.Builder(autofillPrivateWindowEntry)
                        .setTitle(context.getString(R.string.prefs_autofill_private_window_title))
                        .setSummary(
                                context.getString(R.string.prefs_autofill_private_window_summary))
                        .setArguments(createSettingsArgs())
                        .build());
    }

    private static void updateMainAutofillAndPasswordsEntry(SettingsIndexData indexData) {
        SettingsIndexData.Entry mainAutofillAndPasswordsEntry =
                indexData.getEntryForKey(
                        MainSettings.class.getName(), MainSettings.PREF_AUTOFILL_AND_PASSWORDS);
        if (mainAutofillAndPasswordsEntry == null) {
            return;
        }

        indexData.updateEntry(
                mainAutofillAndPasswordsEntry.id,
                new SettingsIndexData.Entry.Builder(mainAutofillAndPasswordsEntry)
                        .setFragment(HomeOfTransactionsFragment.class.getName())
                        .setArguments(createHomeOfTransactionsArgs())
                        .build());
    }

    private static void updateLegacyAutofillOptionsEntry(
            Context context, SettingsIndexData indexData) {
        SettingsIndexData.Entry legacyAutofillOptionsEntry =
                indexData.getEntryForKey(
                        MainSettings.class.getName(), MainSettings.PREF_AUTOFILL_OPTIONS);
        if (legacyAutofillOptionsEntry == null) {
            return;
        }

        indexData.updateEntry(
                legacyAutofillOptionsEntry.id,
                new SettingsIndexData.Entry.Builder(legacyAutofillOptionsEntry)
                        .setTitle(AutofillOptionsMediator.getFragmentTitle(context))
                        .setFragment(AutofillOptionsFragment.class.getName())
                        .setArguments(createSettingsArgs())
                        .build());
    }

    private static void updateHomeOfTransactionsAutofillOptionsEntry(SettingsIndexData indexData) {
        SettingsIndexData.Entry homeOfTransactionsAutofillOptionsEntry =
                indexData.getEntryForKey(
                        HomeOfTransactionsFragment.class.getName(),
                        HomeOfTransactionsFragment.PREF_AUTOFILL_SETTINGS);
        if (homeOfTransactionsAutofillOptionsEntry == null) {
            return;
        }

        indexData.updateEntry(
                homeOfTransactionsAutofillOptionsEntry.id,
                new SettingsIndexData.Entry.Builder(homeOfTransactionsAutofillOptionsEntry)
                        .setFragment(AutofillOptionsFragment.class.getName())
                        .setArguments(createSettingsArgs())
                        .build());
    }

    private static void addAutofillOptionsChildParentLinks(SettingsIndexData indexData) {
        indexData.addChildParentLink(
                HomeOfTransactionsFragment.class.getName(),
                PreferenceParser.createUniqueId(
                        MainSettings.class.getName(), MainSettings.PREF_AUTOFILL_AND_PASSWORDS));
        indexData.addChildParentLink(
                AutofillOptionsFragment.class.getName(),
                PreferenceParser.createUniqueId(
                        HomeOfTransactionsFragment.class.getName(),
                        HomeOfTransactionsFragment.PREF_AUTOFILL_SETTINGS));
    }

    /**
     * Updates Brave's Autofill private-window search entry after upstream providers populate or
     * refresh {@link SettingsIndexData}.
     *
     * <p>This keeps the moved private-window preference under {@link AutofillOptionsFragment},
     * rewires the legacy Autofill options and Home of Transactions routes with their required
     * launch arguments, and restores the child-parent links used for breadcrumb resolution.
     */
    public static void updateSearchIndexEntries(Context context, SettingsIndexData indexData) {
        updateAutofillPrivateWindowEntry(context, indexData);
        updateMainAutofillAndPasswordsEntry(indexData);
        updateLegacyAutofillOptionsEntry(context, indexData);
        updateHomeOfTransactionsAutofillOptionsEntry(indexData);
        addAutofillOptionsChildParentLinks(indexData);
    }
}
