/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.autofill.settings.options;

import static org.chromium.build.NullUtil.assumeNonNull;

import android.content.Context;
import android.os.Bundle;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.VisibleForTesting;
import androidx.preference.Preference;
import androidx.preference.PreferenceViewHolder;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.autofill.brave.R;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.settings.ChromeBaseSettingsFragment;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;
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
                new AutofillPrivateWindowPreference(getPreferenceManager().getContext());
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

    /**
     * Temporary workaround to ensure the preference text of the Brave backed preference aligns with
     * the visual positioning of the labels of the existing chromium radio button items.
     *
     * <p>The intent is to rework the radio button items to place the indicator on the end side of
     * the text, at which point the text inset can be standardized and this workaround can be
     * removed.
     */
    private static class AutofillPrivateWindowPreference extends ChromeSwitchPreference {
        AutofillPrivateWindowPreference(Context context) {
            super(context);
        }

        @Override
        public void onBindViewHolder(PreferenceViewHolder holder) {
            super.onBindViewHolder(holder);

            View iconFrame =
                    holder.findViewById(
                            org.chromium.components.browser_ui.settings.R.id.icon_frame);
            if (iconFrame == null) return;

            ViewGroup.LayoutParams layoutParams = iconFrame.getLayoutParams();
            if (layoutParams == null) return;

            int alignedStartSlotWidth =
                    getContext()
                            .getResources()
                            .getDimensionPixelSize(org.chromium.ui.R.dimen.min_touch_target_size);
            if (layoutParams.width == alignedStartSlotWidth) return;

            layoutParams.width = alignedStartSlotWidth;
            iconFrame.setLayoutParams(layoutParams);
        }
    }
}
