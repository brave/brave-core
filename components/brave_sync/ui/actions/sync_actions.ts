/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { action } from 'typesafe-actions'

// Constants
import { types } from '../constants/sync_types'

export const doSomethingSync = (something: string) => action(types.TEST_SYNC_ACTION, {
  something
})

/**
 * Dispatches a message when sync init data needs to be saved
 * @param {Array.<number>|null} seed
 */
export const saveSyncInitData = (/*seed: any*/) => {
  // TODO
}

/**
 * Dispatches a message when sync needs to be restarted
 * @param {Array.<number>|null} seed
 */
export const reloadSyncExtension = () => {
  // TODO
}

/**
 * Dispatches a message to reset Sync data on this device and the cloud.
 */
export const resetSync = () => {
  // TODO
}
