/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

export const enum types {
  CREATE_WALLET = '@@rewards/CREATE_WALLET',
  WALLET_CREATED = '@@rewards/WALLET_CREATED',
  WALLET_CREATE_FAILED = '@@rewards/WALLET_CREATE_FAILED',
  ON_SETTING_SAVE = '@@rewards/ON_SETTING_SAVE',
  ON_WALLET_PROPERTIES = '@@rewards/ON_WALLET_PROPERTIES',
  GET_WALLET_PROPERTIES = '@@rewards/GET_WALLET_PROPERTIES',
  GET_PROMOTION = '@@rewards/GET_PROMOTION',
  ON_PROMOTION = '@@rewards/ON_PROMOTION',
  GET_PROMOTION_CAPTCHA = '@@rewards/GET_PROMOTION_CAPTCHA',
  ON_PROMOTION_CAPTCHA = '@@rewards/ON_PROMOTION_CAPTCHA',
  ON_PROMOTION_RESET = '@@rewards/ON_PROMOTION_RESET',
  ON_PROMOTION_DELETE = '@@rewards/ON_PROMOTION_DELETE'
}
