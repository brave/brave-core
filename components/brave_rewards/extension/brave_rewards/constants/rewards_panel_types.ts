/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

export const enum types {
  CREATE_WALLET = '@@rewards_panel/CREATE_WALLET',
  ON_WALLET_CREATED = '@@rewards_panel/ON_WALLET_CREATED',
  ON_WALLET_CREATE_FAILED = '@@rewards_panel/ON_WALLET_CREATE_FAILED',
  ON_TAB_ID = '@@rewards_panel/ON_TAB_ID',
  ON_TAB_RETRIEVED = '@@rewards_panel/ON_TAB_RETRIEVED',
  ON_PUBLISHER_DATA = '@@rewards_panel/ON_PUBLISHER_DATA',
  GET_WALLET_PROPERTIES = '@@rewards_panel/GET_WALLET_PROPERTIES',
  ON_WALLET_PROPERTIES = '@@rewards_panel/ON_WALLET_PROPERTIES',
  GET_CURRENT_REPORT = '@@rewards_panel/GET_CURRENT_REPORT',
  ON_CURRENT_REPORT = '@@rewards_panel/ON_CURRENT_REPORT'
}
