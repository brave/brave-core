/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// Types
import * as Shields from '../../types/state/shieldsPannelState'
import { SettingsData } from '../../types/other/settingsTypes'

// Helpers
import { debounce } from '../../../../../common/debounce'
const keyName = 'shields-persistent-data'

export const initialSettingsData: SettingsData = {
  showAdvancedView: false,
  statsBadgeVisible: true
}

export const defaultPersistentData: Shields.PersistentData = {
  isFirstAccess: true
}

export const loadPersistentData = (): Shields.PersistentData => {
  const data = window.localStorage.getItem(keyName)
  let state: Shields.PersistentData = defaultPersistentData
  if (data) {
    try {
      state = JSON.parse(data)
    } catch (e) {
      console.error('[Shields] Could not parse local storage data: ', e)
    }
  }
  return state
}

export const savePersistentDataDebounced = debounce((data: Shields.PersistentData) => {
  if (data) {
    window.localStorage.setItem(keyName, JSON.stringify(data))
  }
}, 50)
