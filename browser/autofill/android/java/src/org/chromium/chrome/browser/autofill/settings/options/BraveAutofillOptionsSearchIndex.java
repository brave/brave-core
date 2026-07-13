/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.autofill.settings.options;

import android.content.Context;
import android.os.Bundle;

import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.autofill.brave.R;
import org.chromium.components.browser_ui.settings.search.PreferenceParser;
import org.chromium.components.browser_ui.settings.search.SettingsIndexData;

@NullMarked
public final class BraveAutofillOptionsSearchIndex {
    private BraveAutofillOptionsSearchIndex() {}

    public static void updateSearchIndexEntries(
            Context context, SettingsIndexData indexData, SettingsRoutes routes) {
        updateAutofillPrivateWindowEntry(context, indexData);
        updateMainAutofillAndPasswordsEntry(indexData, routes);
        updateLegacyAutofillOptionsEntry(context, indexData, routes);
        updateHomeOfTransactionsAutofillOptionsEntry(indexData, routes);
        addAutofillOptionsChildParentLinks(indexData, routes);
    }

    public static final class SettingsRoutes {
        private final String mMainSettingsFragmentName;
        private final String mMainAutofillAndPasswordsKey;
        private final String mLegacyAutofillOptionsKey;
        private final String mHomeOfTransactionsFragmentName;
        private final String mHomeOfTransactionsAutofillSettingsKey;
        private final String mHomeOfTransactionsReferrerExtra;
        private final int mHomeOfTransactionsSettingsMenuReferrer;

        public SettingsRoutes(
                String mainSettingsFragmentName,
                String mainAutofillAndPasswordsKey,
                String legacyAutofillOptionsKey,
                String homeOfTransactionsFragmentName,
                String homeOfTransactionsAutofillSettingsKey,
                String homeOfTransactionsReferrerExtra,
                int homeOfTransactionsSettingsMenuReferrer) {
            mMainSettingsFragmentName = mainSettingsFragmentName;
            mMainAutofillAndPasswordsKey = mainAutofillAndPasswordsKey;
            mLegacyAutofillOptionsKey = legacyAutofillOptionsKey;
            mHomeOfTransactionsFragmentName = homeOfTransactionsFragmentName;
            mHomeOfTransactionsAutofillSettingsKey = homeOfTransactionsAutofillSettingsKey;
            mHomeOfTransactionsReferrerExtra = homeOfTransactionsReferrerExtra;
            mHomeOfTransactionsSettingsMenuReferrer = homeOfTransactionsSettingsMenuReferrer;
        }
    }

    private static Bundle createAutofillOptionsSettingsArgs() {
        return AutofillOptionsFragment.createRequiredArgs(
                AutofillOptionsFragment.AutofillOptionsReferrer.SETTINGS);
    }

    private static Bundle createHomeOfTransactionsArgs(SettingsRoutes routes) {
        Bundle args = new Bundle();
        args.putInt(
                routes.mHomeOfTransactionsReferrerExtra,
                routes.mHomeOfTransactionsSettingsMenuReferrer);
        return args;
    }

    private static void updateAutofillPrivateWindowEntry(
            Context context, SettingsIndexData indexData) {
        SettingsIndexData.Entry autofillPrivateWindowEntry =
                indexData.getEntryForKey(
                        AutofillOptionsFragment.class.getName(),
                        BraveAutofillOptionsFragmentBase.PREF_AUTOFILL_PRIVATE_WINDOW);
        if (autofillPrivateWindowEntry == null) {
            indexData.addEntryForKey(
                    AutofillOptionsFragment.class.getName(),
                    BraveAutofillOptionsFragmentBase.PREF_AUTOFILL_PRIVATE_WINDOW,
                    R.string.prefs_autofill_private_window_title,
                    R.string.prefs_autofill_private_window_summary,
                    createAutofillOptionsSettingsArgs());
            return;
        }

        indexData.updateEntry(
                autofillPrivateWindowEntry.id,
                new SettingsIndexData.Entry.Builder(autofillPrivateWindowEntry)
                        .setTitle(context.getString(R.string.prefs_autofill_private_window_title))
                        .setSummary(
                                context.getString(R.string.prefs_autofill_private_window_summary))
                        .setArguments(createAutofillOptionsSettingsArgs())
                        .build());
    }

    private static void updateMainAutofillAndPasswordsEntry(
            SettingsIndexData indexData, SettingsRoutes routes) {
        SettingsIndexData.Entry mainAutofillAndPasswordsEntry =
                indexData.getEntryForKey(
                        routes.mMainSettingsFragmentName, routes.mMainAutofillAndPasswordsKey);
        if (mainAutofillAndPasswordsEntry == null) {
            return;
        }

        indexData.updateEntry(
                mainAutofillAndPasswordsEntry.id,
                new SettingsIndexData.Entry.Builder(mainAutofillAndPasswordsEntry)
                        .setFragment(routes.mHomeOfTransactionsFragmentName)
                        .setArguments(createHomeOfTransactionsArgs(routes))
                        .build());
    }

    private static void updateLegacyAutofillOptionsEntry(
            Context context, SettingsIndexData indexData, SettingsRoutes routes) {
        SettingsIndexData.Entry legacyAutofillOptionsEntry =
                indexData.getEntryForKey(
                        routes.mMainSettingsFragmentName, routes.mLegacyAutofillOptionsKey);
        if (legacyAutofillOptionsEntry == null) {
            return;
        }

        indexData.updateEntry(
                legacyAutofillOptionsEntry.id,
                new SettingsIndexData.Entry.Builder(legacyAutofillOptionsEntry)
                        .setTitle(AutofillOptionsMediator.getFragmentTitle(context))
                        .setFragment(AutofillOptionsFragment.class.getName())
                        .setArguments(createAutofillOptionsSettingsArgs())
                        .build());
    }

    private static void updateHomeOfTransactionsAutofillOptionsEntry(
            SettingsIndexData indexData, SettingsRoutes routes) {
        SettingsIndexData.Entry homeOfTransactionsAutofillOptionsEntry =
                indexData.getEntryForKey(
                        routes.mHomeOfTransactionsFragmentName,
                        routes.mHomeOfTransactionsAutofillSettingsKey);
        if (homeOfTransactionsAutofillOptionsEntry == null) {
            return;
        }

        indexData.updateEntry(
                homeOfTransactionsAutofillOptionsEntry.id,
                new SettingsIndexData.Entry.Builder(homeOfTransactionsAutofillOptionsEntry)
                        .setFragment(AutofillOptionsFragment.class.getName())
                        .setArguments(createAutofillOptionsSettingsArgs())
                        .build());
    }

    private static void addAutofillOptionsChildParentLinks(
            SettingsIndexData indexData, SettingsRoutes routes) {
        indexData.addChildParentLink(
                routes.mHomeOfTransactionsFragmentName,
                PreferenceParser.createUniqueId(
                        routes.mMainSettingsFragmentName, routes.mMainAutofillAndPasswordsKey));
        indexData.addChildParentLink(
                AutofillOptionsFragment.class.getName(),
                PreferenceParser.createUniqueId(
                        routes.mHomeOfTransactionsFragmentName,
                        routes.mHomeOfTransactionsAutofillSettingsKey));
    }
}
