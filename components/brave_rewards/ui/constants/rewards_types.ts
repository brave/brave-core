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
  ON_GRANT = '@@rewards/ON_GRANT',
  GET_PROMOTION_CAPTCHA = '@@rewards/GET_PROMOTION_CAPTCHA',
  ON_GRANT_CAPTCHA = '@@rewards/ON_GRANT_CAPTCHA',
  SOLVE_PROMOTION_CAPTCHA = '@@rewards/SOLVE_PROMOTION_CAPTCHA',
  ON_GRANT_RESET = '@@rewards/ON_GRANT_RESET',
  ON_GRANT_DELETE = '@@rewards/ON_GRANT_DELETE',
  ON_GRANT_FINISH = '@@rewards/ON_GRANT_FINISH',
  GET_WALLLET_PASSPHRASE = '@@rewards/GET_WALLLET_PASSPHRASE',
  ON_WALLLET_PASSPHRASE = '@@rewards/ON_WALLLET_PASSPHRASE',
  RECOVER_WALLET = '@@rewards/RECOVER_WALLET',
  ON_RECOVER_WALLET_DATA = '@@rewards/ON_RECOVER_WALLET_DATA',
  ON_MODAL_BACKUP_CLOSE = '@@rewards/ON_MODAL_BACKUP_CLOSE',
  ON_MODAL_BACKUP_OPEN = '@@rewards/ON_MODAL_BACKUP_OPEN',
  ON_CLEAR_RECOVERY = '@@rewards/ON_CLEAR_RECOVERY',
  ON_RECONCILE_STAMP = '@@rewards/ON_RECONCILE_STAMP'
}
