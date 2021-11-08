/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// Types
import * as types from '../constants/settingsTypes'
import * as actions from '../types/actions/settingsActions'
import { SettingsOptions, GeneratedSettingsKey, SettingsData } from '../types/other/settingsTypes'

// Helpers
import * as shieldsAPI from '../background/api/shieldsAPI'
import * as settingsUtils from '../helpers/settingsUtils'
import { areObjectsEqual } from '../helpers/objectUtils'
import { Dispatch } from 'redux'
import { State } from '../types/state/mainState'
/**
 * Inform the store that settings have changed. This action is used only
 * for storing values in Redux and does not tell which brave settings have changed.
 */
export const setStoreSettingsChange: actions.SetStoreSettingsChange = (settingsData) => {
  return {
    type: types.SET_STORE_SETTINGS_CHANGE,
    settingsData
  }
}

/**
 * Perform an update in settings both in brave://settings and Shields store whenever a setting change.
 * This action is bounded to the settings listener and should not be used outside this scope.
 */
export const settingsDidChange: actions.SettingsDidChange = (settings) => {
  const settingsOptions: SettingsOptions = settingsUtils.settingsOptions
  const currentSetting: Partial<GeneratedSettingsKey> = settingsOptions[settings.key]
  return setStoreSettingsChange({ [currentSetting]: settings.value })
}

/**
 * Get a list of settings values from brave://settings and update if comparison
 * against settings values from store deosn't match.
 */
type FetchAndDispatchSettings = () => (dispatch: Dispatch, getState: () => State) => void

export const fetchAndDispatchSettings: FetchAndDispatchSettings = () => {
  return (dispatch, getState) => {
    const settingsDataFromStore: SettingsData = getState().shieldsPanel.settingsData
    shieldsAPI.getViewPreferences()
      .then(
        (settingsData: SettingsData) => {
          if (!areObjectsEqual(settingsDataFromStore, settingsData)) {
            dispatch(setStoreSettingsChange(settingsData))
          }
        },
        error => console.error('[Shields] error updating settings data', error)
      )
  }
}
