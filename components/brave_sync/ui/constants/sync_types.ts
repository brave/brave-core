/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

export const enum types {
  // remove
  TEST_SYNC_ACTION = '@@sync/TEST_SYNC_ACTION',
  // remove abpve
  APP_CREATE_SYNC_CACHE = '@@sync/APP_CREATE_SYNC_CACHE',
  APP_PENDING_SYNC_RECORDS_ADDED = '@@sync/APP_PENDING_SYNC_RECORDS_ADDED',
  APP_PENDING_SYNC_RECORDS_REMOVED = '@@sync/APP_PENDING_SYNC_RECORDS_REMOVED',
  APP_SAVE_SYNC_DEVICES = '@@sync/APP_SAVE_SYNC_DEVICES',
  APP_SAVE_SYNC_INIT_DATA = '@@sync/APP_SAVE_SYNC_INIT_DATA',
  APP_RESET_SYNC_DATA = '@@sync/APP_RESET_SYNC_DATA',
  APP_SET_SYNC_SETUP_ERROR = '@@sync/APP_SET_SYNC_SETUP_ERROR'
}
