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
  ON_CURRENT_REPORT = '@@rewards_panel/ON_CURRENT_REPORT',
  ON_NOTIFICATION_ADDED = '@@rewards_panel/ON_NOTIFICATION_ADDED',
  ON_NOTIFICATION_DELETED = '@@rewards_panel/ON_NOTIFICATION_DELETED',
  DELETE_NOTIFICATION = '@@rewards_panel/DELETE_NOTIFICATION',
  INCLUDE_IN_AUTO_CONTRIBUTION = '@@rewards_panel/INCLUDE_IN_AUTO_CONTRIBUTION',
  GET_GRANT = '@@rewards_panel/GET_GRANT',
  ON_GRANT = '@@rewards_panel/ON_GRANT',
  GET_GRANT_CAPTCHA = '@@rewards_panel/GET_GRANT_CAPTCHA',
  ON_GRANT_CAPTCHA = '@@rewards_panel/ON_GRANT_CAPTCHA',
  SOLVE_GRANT_CAPTCHA = '@@rewards_panel/SOLVE_GRANT_CAPTCHA',
  ON_GRANT_RESET = '@@rewards_panel/ON_GRANT_RESET',
  ON_GRANT_DELETE = '@@rewards_panel/ON_GRANT_DELETE',
  ON_GRANT_FINISH = '@@rewards_panel/ON_GRANT_FINISH',
}
