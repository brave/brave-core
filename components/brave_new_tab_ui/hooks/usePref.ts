// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import { addChangeListener, getPreferences, sendSavePref } from '../api/preferences'
import { PrefHookManager, createPrefsHook } from './PrefHookManager'

/**
 * The pref manager. Exported for use from stories.
 */
export const newTabPrefManager = new PrefHookManager(getPreferences, sendSavePref, addChangeListener)

/**
 * A hook for subscribing to and updating |NewTab.Preferences|.
 * @param pref The name of the pref.
 * @returns An array with two items, the current value of the pref, and a function
 * for updating the pref.
 */
export const useNewTabPref = createPrefsHook(newTabPrefManager)
