/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.components.browser_ui.settings;

import androidx.preference.Preference;
import androidx.preference.PreferenceScreen;

import java.util.ArrayList;

public class BraveSettingsUtils {
    /**
     * Replacement for {@link SettingsUtils#getVisiblePreferences} that sorts only top-level items
     * (direct children of the PreferenceScreen) before recursing into their children.
     *
     * <p>{@code getPreference(i)} returns preferences in insertion order, not sorted order. The
     * {@code PreferenceGroupAdapter} calls {@code sortPreferences()} before rendering, but callers
     * of {@code getVisiblePreferences} (e.g. {@code applyContainmentForFragment}) may run before
     * that sort happens. A global flat sort would mix order values across parent/child scopes —
     * children with {@code DEFAULT_ORDER = Integer.MAX_VALUE} would be pushed to the end of the
     * flat list, misaligning containment styles with the adapter's actual view positions.
     */
    public static ArrayList<Preference> getVisiblePreferences(PreferenceScreen preferenceScreen) {
        ArrayList<Preference> visiblePreferences = new ArrayList<>();
        if (preferenceScreen == null) return visiblePreferences;

        ArrayList<Preference> topLevel = new ArrayList<>();
        for (int i = 0; i < preferenceScreen.getPreferenceCount(); i++) {
            Preference preference = preferenceScreen.getPreference(i);
            if (preference.isVisible()) {
                topLevel.add(preference);
            }
        }
        topLevel.sort((a, b) -> Integer.compare(a.getOrder(), b.getOrder()));
        for (Preference preference : topLevel) {
            SettingsUtils.addVisiblePreferences(preference, visiblePreferences);
        }
        return visiblePreferences;
    }
}
