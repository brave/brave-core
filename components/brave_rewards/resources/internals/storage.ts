/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

const defaultState: RewardsInternals.State = {
  balance: {
    total: 0.0,
    wallets: {}
  },
  isRewardsEnabled: false,
  info: {
    isKeyInfoSeedValid: false,
    walletPaymentId: '',
    bootStamp: 0
  },
  contributions: [],
  promotions: [],
  log: '',
  fullLog: '',
  externalWallet: {
    address: '',
    status: 0
  },
  eventLogs: []
}

export const load = (): RewardsInternals.State => {
  return defaultState
}
