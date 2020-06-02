/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as SettingsPrivate from '../../../../../common/settingsPrivate'

const ampSettingsKey = 'brave.shields.amp_neutralized'

export async function getAMPNeutralized (): Promise<boolean> {
  const pref = await SettingsPrivate.getPreference(ampSettingsKey)
  if (pref.type !== chrome.settingsPrivate.PrefType.BOOLEAN) {
    throw new Error(`Unexpected settings type received for "${ampSettingsKey}.` +
      `" Expected: ${chrome.settingsPrivate.PrefType.BOOLEAN}, Received: ${pref.type}`)
  }
  return pref.value
}
