/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import settingsActions from '../actions/settingsActions'
import { settingsKeyList } from '../../helpers/settingsUtils'
import { SettingsKey } from '../../types/other/settingsTypes'

chrome.settingsPrivate.onPrefsChanged.addListener(function (settings) {
  const settingsKey = settings[0].key as SettingsKey
  // only call the store update if the settings change is something we care about
  if (settingsKeyList.includes(settingsKey)) {
    settingsActions.settingsDidChange(settings[0])
  }
})
