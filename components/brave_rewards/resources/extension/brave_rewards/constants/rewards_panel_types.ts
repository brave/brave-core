/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

export const enum types {
  CREATE_WALLET = '@@rewards_panel/CREATE_WALLET',
  ON_WALLET_INITIALIZED = '@@rewards_panel/ON_WALLET_INITIALIZED',
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
  GET_GRANTS = '@@rewards_panel/GET_GRANTS',
  ON_GRANT = '@@rewards_panel/ON_GRANT',
  GET_GRANT_CAPTCHA = '@@rewards_panel/GET_GRANT_CAPTCHA',
  ON_GRANT_CAPTCHA = '@@rewards_panel/ON_GRANT_CAPTCHA',
  SOLVE_GRANT_CAPTCHA = '@@rewards_panel/SOLVE_GRANT_CAPTCHA',
  ON_GRANT_RESET = '@@rewards_panel/ON_GRANT_RESET',
  ON_GRANT_DELETE = '@@rewards_panel/ON_GRANT_DELETE',
  ON_GRANT_FINISH = '@@rewards_panel/ON_GRANT_FINISH',
  ON_PENDING_CONTRIBUTIONS_TOTAL = '@@rewards_panel/ON_PENDING_CONTRIBUTIONS_TOTAL',
  ON_ENABLED_MAIN = '@@rewards_panel/ON_ENABLED_MAIN',
  ON_ENABLED_AC = '@@rewards_panel/ON_ENABLED_AC',
  ON_PUBLISHER_LIST_NORMALIZED = '@@rewards_panel/ON_PUBLISHER_LIST_NORMALIZED',
  ON_EXCLUDED_SITES_CHANGED = '@@rewards_panel/ON_EXCLUDED_SITES_CHANGED',
  ON_SETTING_SAVE = '@@rewards_panel/ON_SETTING_SAVE',
  SAVE_RECURRING_TIP = '@@rewards_panel/SAVE_RECURRING_TIP',
  REMOVE_RECURRING_TIP = '@@rewards_panel/REMOVE_RECURRING_TIP',
  ON_RECURRING_TIPS = '@@rewards_panel/ON_RECURRING_TIPS',
  ON_PUBLISHER_BANNER = '@@rewards_panel/ON_PUBLISHER_BANNER',
  ON_PUBLISHER_STATUS_REFRESHED = '@@rewards_panel/ON_PUBLISHER_STATUS_REFRESHED'
}

// Note: This declaration must match the RewardsNotificationType enum in
// brave/components/brave_rewards/browser/rewards_notification_service.h
export const enum RewardsNotificationType {
  REWARDS_NOTIFICATION_INVALID = 0,
  REWARDS_NOTIFICATION_AUTO_CONTRIBUTE,
  REWARDS_NOTIFICATION_GRANT,
  REWARDS_NOTIFICATION_GRANT_ADS,
  REWARDS_NOTIFICATION_FAILED_CONTRIBUTION,
  REWARDS_NOTIFICATION_IMPENDING_CONTRIBUTION,
  REWARDS_NOTIFICATION_INSUFFICIENT_FUNDS,
  REWARDS_NOTIFICATION_BACKUP_WALLET,
  REWARDS_NOTIFICATION_TIPS_PROCESSED
}
